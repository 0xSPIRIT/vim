void vim_start_of_file(Vim_Instance *vim) {
    vim->x = 0;
    vim->y = 0;
}

void vim_end_of_file(Vim_Instance *vim) {
    vim->x = 0;
    vim->y = (int)vim->line_count-1;
}

void vim_clamp_cursor(Vim_Instance *vim, Vim_Mode mode) {
    if (vim->y < 0) {
        vim->y = 0;
    } else if (vim->y >= vim->line_count) {
        vim->y = (int)vim->line_count-1;
    }
    
    if (vim->lines[vim->y].length == 0) {
        vim->x = 0;
        return;
    }

    if (vim->x < 0) {
        vim->x = 0;
    } else if (mode == VIM_INSERT && vim->x > vim->lines[vim->y].length) {
        vim->x = (int)vim->lines[vim->y].length;
    } else if (mode != VIM_INSERT && vim->x >= vim->lines[vim->y].length) {
        vim->x = (int)vim->lines[vim->y].length-1;
    }
}

void vim_start_of_line(Vim_Instance *vim) {
    vim->x = 0;
}

void vim_end_of_line(Vim_Instance *vim) {
    vim->x = (int)vim->lines[vim->y].length;
    if (vim->mode != VIM_INSERT) vim->x--;
    
    vim_clamp_cursor(vim, vim->mode);
}

void vim_up(Vim_Instance *vim) {
    vim->y--;
    vim_clamp_cursor(vim, vim->mode);
}

void vim_down(Vim_Instance *vim) {
    vim->y++;
    vim_clamp_cursor(vim, vim->mode);
}

void vim_left(Vim_Instance *vim) {
    vim->x--;
    vim_clamp_cursor(vim, vim->mode);
}

void vim_right(Vim_Instance *vim) {
    vim->x++;
    vim_clamp_cursor(vim, vim->mode);
}

bool vim_is_line_empty(String line) {
    if (line.length == 0) return true;
    
    for (int i = 0; i < line.length; i++) {
        if (line.buffer[i] != ' ') return false;
    }
    
    return true;
}

void vim_move_down_to_empty_line(Vim_Instance *vim) {
    while (vim->y < vim->line_count &&  vim_is_line_empty(vim->lines[vim->y])) vim->y++;
    while (vim->y < vim->line_count && !vim_is_line_empty(vim->lines[vim->y])) vim->y++;
    vim_clamp_cursor(vim, vim->mode);
}

void vim_move_up_to_empty_line(Vim_Instance *vim) {
    while (vim->y >= 0 &&  vim_is_line_empty(vim->lines[vim->y])) vim->y--;
    while (vim->y >= 0 && !vim_is_line_empty(vim->lines[vim->y])) vim->y--;
    vim_clamp_cursor(vim, vim->mode);
}

void vim_move_forward_word(Vim_Instance *vim) {
    String *line = &vim->lines[vim->y];
    
    if (!line->length) return;
    
    while (vim->x < line->length-1 && line->buffer[vim->x] != ' ') vim->x++;
    while (vim->x < line->length-1 && line->buffer[vim->x] == ' ') vim->x++;
}

void vim_move_backward_word(Vim_Instance *vim) {
    String *line = &vim->lines[vim->y];
    
    if (!line->length) return;
    
    if (vim->x > 0 && line->buffer[vim->x-1] == ' ') vim->x--;
    while (vim->x > 0 && line->buffer[vim->x-1] != ' ') vim->x--;
    
    while (vim->x > 0 && line->buffer[vim->x  ] == ' ') vim->x--;
    while (vim->x > 0 && line->buffer[vim->x-1] != ' ') vim->x--;
}

void vim_move_end_word(Vim_Instance *vim) {
    String *line = &vim->lines[vim->y];
    
    while (vim->x < line->length-1 && line->buffer[vim->x+1] == ' ') vim->x++;
    while (vim->x < line->length-1 && line->buffer[vim->x+1] != ' ') vim->x++;
}

void vim_move_to_first_non_whitespace(Vim_Instance *vim) {
    String *line = &vim->lines[vim->y];
    
    vim->x = 0;
    while (vim->x < line->length && line->buffer[vim->x] == ' ') vim->x++;
}

void vim_newline(Vim_Instance *vim) {
    String end = {0};
    
    if (vim->lines[vim->y].length > 0 && vim->x < vim->lines[vim->y].length) {
        end = string_slice_duplicate(vim->lines[vim->y], vim->x, vim->lines[vim->y].length-1);
        string_delete_range(&vim->lines[vim->y], vim->x, vim->lines[vim->y].length-1);
    }
    
    vim->line_count++;
    if (vim->line_count > vim->line_capacity) {
        vim_reallocate_lines(vim);
    }
    
    vim->y++;
    
    // Shift over to make space for the new line
    for (u64 i = vim->line_count-2; i >= vim->y; i--) {
        string_copy(&vim->lines[i+1], vim->lines[i]);
    }
    
    string_clear(&vim->lines[vim->y]);
    if (end.length) {
        string_copy(&vim->lines[vim->y], end);
    }
    
    vim->x = 0;
    
    vim_clamp_cursor(vim, VIM_INSERT);
}

void vim_delete_char(Vim_Instance *vim) {
    if (vim->x == vim->lines[vim->y].length) return;
    string_delete_char(&vim->lines[vim->y], vim->x);
}

void vim_delete_line(Vim_Instance *vim, u64 y) {
    vim->line_count--;
    for (u64 i = y; i < vim->line_count; i++) {
        string_copy(&vim->lines[i], vim->lines[i+1]);
    }
}

void vim_backspace(Vim_Instance *vim) {
    if (vim->x > 0) {
        string_delete_char(&vim->lines[vim->y], --vim->x);
    } else if (vim->y > 0) {
        if (vim->lines[vim->y].length > 0) {
            String end = string_slice_duplicate(vim->lines[vim->y], 0, vim->lines[vim->y].length-1);
            
            vim_delete_line(vim, vim->y--);
            vim->x = (int)vim->lines[vim->y].length;
            
            string_append(&vim->lines[vim->y], end);
        } else {
            vim_delete_line(vim, vim->y--);
            vim->x = (int)vim->lines[vim->y].length;
        }
    }
}

void vim_concatenate_lines(Vim_Instance *vim) {
    if (vim->line_count == 1) return;
    
    vim_down(vim);
    vim_start_of_line(vim);
    vim_backspace(vim);
}

void vim_insert_mode_newline_after(Vim_Instance *vim) {
    vim_mode(vim, VIM_INSERT);
    vim_end_of_line(vim);
    vim_newline(vim);
}

void vim_insert_mode_newline_before(Vim_Instance *vim) {
    vim_mode(vim, VIM_INSERT);
    vim_start_of_line(vim);
    vim_newline(vim);
    vim_up(vim);
}

void vim_insert_mode_next_char(Vim_Instance *vim) {
    vim_mode(vim, VIM_INSERT);
    vim_right(vim);
}

void vim_end_of_line_insert_mode(Vim_Instance *vim) {
    vim_mode(vim, VIM_INSERT);
    vim_end_of_line(vim);
}

void vim_start_of_line_insert_mode(Vim_Instance *vim) {
    vim_mode(vim, VIM_INSERT);
    vim_start_of_line(vim);
}

void vim_tab(Vim_Instance *vim) {
    String tab = cs("    ");
    vim->x = string_insert(&vim->lines[vim->y], vim->x, tab);
}

////////////////////////////////////

void vim_delete(Vim_Instance *vim, Vim_Range range) {
    range = vim_fix_range(range);
    
    u64 start_x = range.start_x;
    u64 start_y = range.start_y;
    
    for (u64 y = range.start_y; y <= range.end_y; y++) {
        String *line = &vim->lines[y];
        
        if (range.start_y == range.end_y) {
            string_delete_range(line, range.start_x, range.end_x);
        }
        
        else if (y == range.start_y) {
            if (range.start_x > 0 && range.start_x < line->length) {
                string_delete_range(line, range.start_x, line->length-1);
            } else if (range.start_x == 0) {
                vim_delete_line(vim, y);
                range.start_y--;
                range.end_y--;
                y--;
            }
        }
        
        else if (y == range.end_y) {
            string_delete_range(line, 0, range.end_x);
        }
        
        else {
            vim_delete_line(vim, y);
            range.start_y--;
            range.end_y--;
            y--;
        }
    }
    
    vim->x = (int)start_x;
    vim->y = (int)start_y;
}

void vim_change(Vim_Instance *vim, Vim_Range range) {
    vim_delete(vim, range);
    vim_mode(vim, VIM_INSERT);
}

void vim_range_start(Vim_Instance *vim) {
    vim->range.start_x = vim->x;
    vim->range.start_y = vim->y;
}

void vim_range_end(Vim_Instance *vim) {
    vim->range.end_x = vim->x;
    vim->range.end_y = vim->y;
}

void vim_change_end_of_line(Vim_Instance *vim) {
    vim_mode(vim, VIM_NORMAL);
    vim_range_start(vim);
    vim_end_of_line(vim);
    vim_range_end(vim);
    vim_change(vim, vim->range);
}

void vim_delete_end_of_line(Vim_Instance *vim) {
    vim_mode(vim, VIM_NORMAL);
    vim_range_start(vim);
    vim_end_of_line(vim);
    vim_range_end(vim);
    vim_delete(vim, vim->range);
}
