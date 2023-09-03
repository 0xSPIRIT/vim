#include <windows.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "util.c"
#include "font.c"

typedef enum Vim_Mode
{
    VIM_NORMAL,
    VIM_INSERT,
    VIM_VISUAL,
    VIM_COMMAND,
    VIM_REPLACE,
    VIM_CHANGE
} Vim_Mode;

typedef struct Vim_Range {
    u64 start_x, start_y;
    u64 end_x, end_y;
} Vim_Range;

typedef struct Vim_Instance
{
    SDL_Window   *window;
    SDL_Renderer *renderer;
    
    Vim_Range range;
    
    bool should_change; // More info for VIM_CHANGE. should we change or just delete
    
    Vim_Mode mode;
    char eat_input;
    
    Font font;
    
    String filename;
    String *lines;
    u64 line_count, line_capacity;
    
    String command_line;
    
    int    argc;
    char **argv;
    
    int w, h;
    
    int stored_x, stored_y; // Previous positions
    int x, y; // Cursor position
    
    int xoff, yoff, yoff_to;
    
    bool closed;
} Vim_Instance;

void vim_init_line(String *string);
void vim_init_lines(Vim_Instance *vim);
void vim_reallocate_lines(Vim_Instance *vim);
void vim_mode(Vim_Instance *vim, Vim_Mode mode);
void vim_eat_input(Vim_Instance *vim, char ch);
Vim_Range vim_fix_range(Vim_Range range);
void vim_backspace_command_char(Vim_Instance *vim);
void vim_write_lines_to_file(String filename, String *lines, u64 line_count);
u64 vim_read_file_to_lines(String filename, Vim_Instance *vim);
void vim_execute_command(Vim_Instance *vim);
void vim_center_screen(Vim_Instance *vim);
void vim_set_yoff(Vim_Instance *vim);

#include "motions.c"
#include "keys.c"

void vim_init_line(String *string) {
    string->capacity = 64;
    string->length   = 0;
    string->buffer   = (char*)allocate(string->capacity);
}

void vim_init_lines(Vim_Instance *vim) {
    vim->line_count = 1;
    vim->line_capacity = 64;
    vim->lines = (String*)allocate(vim->line_capacity * sizeof(String));
    
    for (int i = 0; i < vim->line_capacity; i++) {
        vim_init_line(&vim->lines[i]);
    }
    
    vim->command_line = make_string(SHORT_STRING_CAPACITY);
}

void vim_reallocate_lines(Vim_Instance *vim) {
    vim->line_capacity *= 2;
    vim->lines = (String*)reallocate(vim->lines, vim->line_capacity * sizeof(String));
}

void vim_type_char(Vim_Instance *vim, char ch) {
    string_insert_char(&vim->lines[vim->y], vim->x++, ch);
}

void vim_type_command_char(Vim_Instance *vim, char ch) {
    string_insert_char(&vim->command_line, vim->x++, ch);
}

void vim_replace_char(Vim_Instance *vim, char ch) {
    vim->lines[vim->y].buffer[vim->x] = ch;
}

void vim_backspace_command_char(Vim_Instance *vim) {
    if (vim->command_line.length) {
        vim->command_line.length--;
        vim->x--;
    }
}

void vim_write_lines_to_file(String filename, String *lines, u64 line_count) {
    u64 calculated_length = 0;
    
    for (u64 i = 0; i < line_count; i++) {
        calculated_length += lines[i].length+1; // +1 for \n
    }
    calculated_length--; // For the trailing \n at end of file.
    
    String full_out = make_string(calculated_length); // Just so we don't deal with reallocations.
    
    for (u64 i = 0; i < line_count; i++) {
        string_append(&full_out, lines[i]);
        if (i != line_count-1)
            string_append_char(&full_out, '\n');
    }
    
    File file = open_file(filename, FILE_WRITE);
    write_to_file(file, full_out);
    close_file(file);
}

void vim_type_string(Vim_Instance *vim, String str) {
    for (u64 i = 0; i < str.length; i++) {
        char ch = str.buffer[i];
        
        if (ch == '\r') {
            if (i < str.length-1 && str.buffer[i+1] == '\n') {
                continue;
            } else {
                vim_newline(vim);
                continue;
            }
        }
        if (ch == '\n') {
            vim_newline(vim);
            continue;
        } else if (ch == '\t') {
            vim_tab(vim);
            continue;
        }
        
        vim_type_char(vim, ch);
    }
}

u64 vim_read_file_to_lines(String filename, Vim_Instance *vim) {
    File file = open_file(filename, FILE_READ);
    if (file.invalid) {
        return 0;
    }
    
    String string = read_file(file);
    close_file(file);

    vim_type_string(vim, string);
    vim->x = vim->y = 0;
    
    return vim->line_count;
}

void vim_execute_command(Vim_Instance *vim) {
    if (string_compare(vim->command_line, cs(":q"))) {
        vim->closed = true;
    } else if (string_compare(vim->command_line, cs(":w"))) {
        vim_write_lines_to_file(cs("out.txt"), vim->lines, vim->line_count);
        string_copy(&vim->command_line, cs("Successfuly written to out.txt"));
    } else if (string_compare(vim->command_line, cs(":wq"))) {
        vim_write_lines_to_file(cs("out.txt"), vim->lines, vim->line_count);
        vim->closed = true;
    }
}

void vim_center_screen(Vim_Instance *vim) {
    vim->yoff_to = vim->y*vim->font.h - vim->h/2;
    if (vim->yoff_to < 0) vim->yoff_to = 0;
    
    vim->yoff = vim->yoff_to;
}

bool is_thing_offscreen(int yoff, int thing_y, int screen_h) {
    return (thing_y < yoff) || (thing_y > yoff+screen_h);
}

void vim_set_yoff(Vim_Instance *vim) {
    int cursor_y = vim->y * vim->font.h;
    
    int visible_h = vim->h-vim->font.h*2;
    
    if (cursor_y < vim->yoff_to) vim->yoff_to = cursor_y;
    else if (cursor_y > vim->yoff_to+visible_h) vim->yoff_to = cursor_y - visible_h;
    
    vim->yoff = vim->yoff_to;
}

void vim_draw_cursor(SDL_Renderer *render, Vim_Instance *vim) {
    Font *font = &vim->font;
    
    int fw = font->w;
    int fh = font->h;
    
    int x = vim->x;
    int y = vim->y;
    
    int dy = -vim->yoff;
    
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
    
    switch (vim->mode) {
        case VIM_NORMAL: case VIM_VISUAL: {
            SDL_Rect rect = { x * fw, y * fh + dy, fw, fh };
            SDL_RenderFillRect(render, &rect);
            
            if (vim->x < vim->lines[vim->y].length) {
                char ch = vim->lines[vim->y].buffer[vim->x];
                font_draw_char(render, font, ch, x*fw, y*fh + dy, (SDL_Color){0,0,0,255});
            }
        } break;
        case VIM_INSERT: {
            SDL_Rect rect = { x * fw, y * fh + dy, 2, fh };
            SDL_RenderFillRect(render, &rect);
        } break;
        case VIM_COMMAND: {
            SDL_Rect rect = { x * fw, vim->h - fh, 2, fh };
            SDL_RenderFillRect(render, &rect);
        } break;
        case VIM_CHANGE: case VIM_REPLACE: {
            SDL_Rect rect = { x * fw, y * fh + 3*fh/4 + dy, fw, fh/4 };
            SDL_RenderFillRect(render, &rect);
        } break;
    }
}

Vim_Range vim_fix_range(Vim_Range range) {
    if (range.end_y < range.start_y) {
        u64 ty = range.end_y;
        range.end_y = range.start_y;
        range.start_y = ty;
        
        u64 tx = range.end_x;
        range.end_x = range.start_x;
        range.start_x = tx;
    }
    return range;
}

void vim_draw_selection(SDL_Renderer *render, Vim_Instance *vim) {
    Vim_Range range = vim_fix_range(vim->range);
    
    int fw = vim->font.w;
    int fh = vim->font.h;
    
    int dy = -vim->yoff;
    
    int screen_width = vim->w;
    
    SDL_SetRenderDrawColor(render, 64, 64, 255, 127);
    
    for (u64 y = range.start_y; y <= range.end_y; y++) {
        String *line = &vim->lines[y];
        
        SDL_Rect rect = {0};
        
        u64 chars_offset = 0;
        u64 length = 0;
        
        if (range.start_y == range.end_y) {
            chars_offset = range.start_x;
            length = range.end_x - range.start_x + 1;
            
            rect = (SDL_Rect){
                (int)range.start_x * fw,
                (int)y * fh,
                (int)(range.end_x - range.start_x) * fw,
                fh
            };
        } else if (y == range.start_y) {
            chars_offset = range.start_x;
            length = line->length - range.start_x;
            
            rect = (SDL_Rect){
                (int)range.start_x * fw,
                (int)y * fh,
                screen_width - (int)range.start_x * fw,
                fh
            };
        } else if (y == range.end_y) {
            length = range.end_x+1;
            
            rect = (SDL_Rect){
                0,
                (int)y * fh,
                (int)range.end_x * fw,
                fh
            };
        } else {
            length = line->length;
            
            rect = (SDL_Rect){
                0,
                (int)y * fh,
                screen_width,
                fh
            };
        }
        
        rect.y += dy;
        
        String view_into = {line->buffer+chars_offset, length, length};
        
        SDL_RenderFillRect(render, &rect);
        
        font_draw_string(render, &vim->font, view_into, rect.x, rect.y, (SDL_Color){0,0,0,0});
    }
}

void vim_eat_input(Vim_Instance *vim, char ch) {
    vim->eat_input = ch;
}

void vim_mode(Vim_Instance *vim, Vim_Mode mode) {
    if (vim->mode == mode) return;
    
    if (vim->mode == VIM_COMMAND) {
        vim->x = vim->stored_x;
        vim->y = vim->stored_y;
    }
    
    vim->mode = mode;
    
    if (mode == VIM_COMMAND) {
        vim->stored_x = vim->x;
        vim->stored_y = vim->y;
        vim->x = 0;
        string_clear(&vim->command_line);
    } else if (mode == VIM_CHANGE) {
        vim_range_start(vim);
    }
    
    vim_clamp_cursor(vim, mode);
}

void vim_init(Vim_Instance *vim, int argc, char **argv) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    
    const int scale = 2;
    
    vim->w = vim->h = 400;
    
    //vim->argc = argc;
    //vim->argv = argv;
    
    String filename = {0};
    
    if (argc == 2) {
        filename = as_string(argv[1]);
        if (!file_exists(filename)) {
            // TODO: Output error: file not found
            ExitProcess(1);
        }
    } else {
        // TODO: Output usage using MessageBox?
        ExitProcess(1);
    }
    
    vim->window = SDL_CreateWindow("vim",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   vim->w*scale,
                                   vim->h*scale,
                                   SDL_WINDOW_RESIZABLE);
    vim->renderer = SDL_CreateRenderer(vim->window, -1, SDL_RENDERER_SOFTWARE);
    
    SDL_RenderSetLogicalSize(vim->renderer, vim->w, vim->h);
    
    vim->font = make_font(vim->renderer, "font.png", 8, 13);
    vim->filename = filename;
    
    vim_init_lines(vim);
    vim_read_file_to_lines(vim->filename, vim);
}

void vim_deinit(Vim_Instance *vim) {
    font_free(&vim->font);
    SDL_DestroyWindow(vim->window);
    SDL_DestroyRenderer(vim->renderer);
    SDL_Quit();
    IMG_Quit();
}

void vim_handle_event(Vim_Instance *vim, SDL_Event *event) {
    switch (event->type) {
        case SDL_QUIT: {
            vim->closed = true;
        } break;
        case SDL_KEYDOWN: {
            vim_handle_keypress(vim, event->key.keysym.sym, event->key.keysym.mod);
        } break;
        case SDL_TEXTINPUT: {
            char ch = event->text.text[0];
            
            if (ch == vim->eat_input) {
                vim->eat_input = 0;
                break;
            }
            
            if (vim->mode == VIM_INSERT) {
                vim_type_char(vim, ch);
            } else if (vim->mode == VIM_COMMAND) {
                vim_type_command_char(vim, ch);
            } else if (vim->mode == VIM_REPLACE) {
                vim_replace_char(vim, ch);
                vim_mode(vim, VIM_NORMAL);
            }
        } break;
        case SDL_WINDOWEVENT: {
            if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                const int scale = 2;
                vim->w = event->window.data1 / scale;
                vim->h = event->window.data2 / scale;
                
                SDL_RenderSetLogicalSize(vim->renderer, vim->w, vim->h);
            }
        } break;
    }
}

void vim_draw(Vim_Instance *vim) {
    SDL_Renderer *render = vim->renderer;
    
    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    SDL_RenderClear(render);
    
    int y = -vim->yoff;
    for (int i = 0; i < vim->line_count; i++) {
        if (!is_thing_offscreen(vim->yoff, y+vim->yoff, vim->h)) {
            font_draw_string(render, &vim->font, vim->lines[i], 0, y, (SDL_Color){255, 255, 255});
        }
        y += vim->font.h;
    }
    
    int command_line_y = vim->h - vim->font.h;
    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    SDL_RenderFillRect(render, &(SDL_Rect){0, command_line_y, vim->w, vim->font.h});
    
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
    font_draw_string(render, &vim->font, vim->command_line, 0, command_line_y, (SDL_Color){255,255,255});
    
    if (vim->mode == VIM_VISUAL)
        vim_draw_selection(render, vim);
    
    vim_draw_cursor(render, vim);
    
    SDL_RenderPresent(render);
}

void main() {
    Vim_Instance vim = {0};
    
    win32_SetProcessDpiAware();
    heap_init();
    stdout_init();
    
    char cwd[MAX_PATH];
    
    GetCurrentDirectory(MAX_PATH, cwd);
    
    int argc;
    char **argv;
    get_command_line_args(&argc, &argv);
    
    vim_init(&vim, argc, argv);
    
    while (!vim.closed) {
        SDL_Event event;
        while (!vim.closed && SDL_WaitEvent(&event)) {
            vim_handle_event(&vim, &event);
            vim_draw(&vim);
        }
    }
    
    vim_deinit(&vim);
    
    ExitProcess(0);
}
