#define FONT_GLYPHS 95

typedef struct Font {
    SDL_Texture *texture;
    SDL_Rect glyphs[FONT_GLYPHS];
    
    int w, h;
} Font;

typedef struct XY {
    int x, y;
} XY;


Font make_font(SDL_Renderer *renderer, const char *font_path, int pt_size) {
    Font result = {0};
    
    TTF_Font *font = TTF_OpenFont(font_path, pt_size);
    
    if (!font) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", "Couldn't load font! Exiting...", 0);
        ExitProcess(1);
    }
    
    SDL_Color color = {255, 255, 255, 255};
    
    char string[FONT_GLYPHS+1];
    for (char ch = 32; ch < 32+FONT_GLYPHS; ch++) {
        string[ch-32] = ch;
    }
    string[FONT_GLYPHS] = 0;
    
    SDL_Surface *s = TTF_RenderText_Blended(font, string, color);
    if (s) {
        SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
        result.texture = t;
    }
    
    int cum_x = 0;
    
    for (int i = 0; i < FONT_GLYPHS; i++) {
        int w, h;
        
        char str[] = { string[i], 0 };
        TTF_SizeText(font, str, &w, &h);
        result.glyphs[i] = (SDL_Rect){ cum_x, 0, w, h };
        
        cum_x += w;
        
        if (w > result.w) result.w = w;
        if (h > result.h) result.h = h;
    }
    
    return result;
}

XY font_draw_char(SDL_Renderer *render, Font *font, char ch, int x, int y, SDL_Color color) {
    int idx = ch-32;
    
    SDL_Rect glyph = font->glyphs[idx];
    
    SDL_Rect src = { glyph.x, 0, glyph.w, glyph.h };
    SDL_Rect dst = { x, y, glyph.w, glyph.h };
    
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
    
    return (XY){x+dx, y+dy};
}

void font_draw_string(SDL_Renderer *render, Font *font, String str, int x, int y, SDL_Color color) {
    for (int i = 0; i < str.length; i++) {
        XY p = font_draw_char(render, font, str.buffer[i], x, y, color);
        x = p.x, y = p.y;
    }
}

void font_free(Font *font) {
    SDL_DestroyTexture(font->texture);
}