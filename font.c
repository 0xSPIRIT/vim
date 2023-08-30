typedef struct Font
{
    SDL_Texture *texture;
    int texture_w, texture_h;
    
    int w, h;
} Font;

typedef struct Position
{
    int x, y;
} Position;

Font make_font(SDL_Renderer *render, const char *font, int w, int h) {
    Font result = {0};
    
    SDL_Surface *surf = IMG_Load(font);
    if (!surf) {
        const char *str = SDL_GetError();
        u64 length = strlen(str);
        print((String){ (char*)str, length, 0 });
    }
    
    result.texture = SDL_CreateTextureFromSurface(render, surf);
    if (!result.texture) {
        const char *str = SDL_GetError();
        u64 length = strlen(str);
        print((String){ (char*)str, length, 0 });
    }
    
    result.texture_w = surf->w;
    result.texture_h = surf->h;
    
    SDL_FreeSurface(surf);
    
    result.w = w;
    result.h = h;
    
    return result;
}

Position font_draw_char(SDL_Renderer *render, Font *font, char ch, int x, int y, SDL_Color color) {
    SDL_Rect src = { ch * font->w, 0, font->w, font->h  };
    SDL_Rect dst = { x, y, font->w, font->h };
    
    int dx = dst.w;
    int dy = 0;
    
    if (ch == '\n') {
        dy += dst.h;
    } else if (ch == ' ') {
    } else {
        SDL_SetTextureColorMod(font->texture, color.r, color.g, color.b);
        SDL_RenderCopy(render, font->texture, &src, &dst);
        SDL_SetTextureColorMod(font->texture, 255, 255, 255);
    }
    
    return (Position){x+dx, y+dy};
}

void font_draw_string(SDL_Renderer *render, Font *font, String str, int x, int y, SDL_Color color) {
    for (int i = 0; i < str.length; i++) {
        Position p = font_draw_char(render, font, str.buffer[i], x, y, color);
        x = p.x, y = p.y;
    }
}

void font_free(Font *font) {
    SDL_DestroyTexture(font->texture);
}