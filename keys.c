void eat(Vim_Instance *vim, SDL_Keycode key, bool shift_key) {
    if (key >= SDLK_a && key <= SDLK_z) {
        char start = 'a';
        if (shift_key) start = 'A';
        vim_eat_input(vim, (char) (start + key - SDLK_a));
    }
}

void vim_handle_keypress(Vim_Instance *vim, SDL_Keycode key, u16 mod) {
    bool control = (mod & KMOD_LCTRL)  || (mod & KMOD_RCTRL);
    bool shift   = (mod & KMOD_LSHIFT) || (mod & KMOD_RSHIFT);
    //bool alt     = (mod & KMOD_LALT)   || (mod & KMOD_RALT);
    
    if (key == SDLK_ESCAPE)    vim_mode(vim, VIM_NORMAL);
    if (key == SDLK_c && control) vim_mode(vim, VIM_NORMAL);
    
    if (vim->mode == VIM_SEARCH) {
        if (key == SDLK_RETURN) {
            if (vim->mode == VIM_SEARCH) {
                vim_search(vim, &vim->search);
            }
            
            vim_mode(vim, VIM_NORMAL);
        }
    }
    
    if (vim->mode == VIM_INSERT) {
        if (key == SDLK_RETURN)    vim_newline(vim);
        if (key == SDLK_BACKSPACE) vim_backspace(vim);
        if (key == SDLK_TAB)       vim_tab(vim);
    }
    
    if (vim->mode == VIM_NORMAL || vim->mode == VIM_VISUAL) {
        if (key == SDLK_j && shift) vim_concatenate_lines(vim);
        
        else if (key == SDLK_h || key == SDLK_LEFT)  vim_left(vim);
        else if (key == SDLK_l || key == SDLK_RIGHT) vim_right(vim);
        else if (key == SDLK_k || key == SDLK_UP)    vim_up(vim);
        else if (key == SDLK_j || key == SDLK_DOWN)  vim_down(vim);
        
        if (key == SDLK_0)          vim_start_of_line(vim);
        if (shift && key == SDLK_4) vim_end_of_line(vim);
        
        if (key == SDLK_r) {
            vim_mode(vim, VIM_REPLACE);
        }
        
        if (key == SDLK_SLASH) {
            vim_mode(vim, VIM_SEARCH);
            if (shift) {
                vim->search.backwards = true;
            }
        }
        
        if (vim->search.string && vim->search.string[0] && key == SDLK_n) {
            vim->search.backwards = shift;
            
            if (vim->search.backwards) {
                vim_left_wrap(vim);
            } else {
                vim_right_wrap(vim);
            }
            
            vim->search.sx = vim->x;
            vim->search.sy = vim->y;
            
            if (vim_search(vim, &vim->search)) {
                vim->x = vim->search.x;
                vim->y = vim->search.y;
            } else {
                vim->x = vim->search.sx;
				vim->y = vim->search.sy;
                if (vim->search.backwards) {
                    vim_right_wrap(vim);
                } else {
                    vim_left_wrap(vim);
                }
            }
        }
        
        if (key == SDLK_x) {
            if (shift) {
                vim_backspace(vim);
            } else {
                vim_delete_char(vim);
            }
        }
        
        if (key == SDLK_LEFTBRACKET) {
            vim_move_up_to_empty_line(vim);
        }
        if (key == SDLK_RIGHTBRACKET) {
            vim_move_down_to_empty_line(vim);
        }
        
        if (key == SDLK_z) vim_center_screen(vim);
        
        if (key == SDLK_w) vim_move_forward_word(vim);
        if (key == SDLK_b) vim_move_backward_word(vim);
        if (key == SDLK_e) vim_move_end_word(vim);
        if (key == SDLK_6 && shift) vim_move_to_first_non_whitespace(vim);
        
        if (key == SDLK_f) {
            vim_mode(vim, VIM_FIND);
        }
        
        if (key == SDLK_g) {
            if (shift) {
                vim_end_of_file(vim);
            } else {
                vim_start_of_file(vim);
            }
        }
        
        if (key == SDLK_v) {
            if (vim->mode == VIM_NORMAL) {
                vim_mode(vim, VIM_VISUAL);
                vim_range_start(vim, shift);
            } else {
                vim_mode(vim, VIM_NORMAL);
            }
        }
        
        if (key == SDLK_i) {
            if (shift) {
                vim_start_of_line_insert_mode(vim);
            } else {
                vim_mode(vim, VIM_INSERT);
            }
        }
        
        if (key == SDLK_a) {
            if (shift) {
                vim_end_of_line_insert_mode(vim);
            } else {
                vim_insert_mode_next_char(vim);
            }
        }
        
        if (key == SDLK_o) {
            if (shift) {
                vim_insert_mode_newline_before(vim);
            } else {
                vim_insert_mode_newline_after(vim);
            }
        }
        
        if (key == SDLK_SEMICOLON && shift) {
            vim_mode(vim, VIM_COMMAND);
        }
        
        if (vim->mode == VIM_VISUAL && key == SDLK_d) {
            vim_delete_range(vim, vim->range);
            vim_mode(vim, VIM_NORMAL);
        }
        
        else if (vim->mode == VIM_NORMAL && key == SDLK_c && !control) {
            if (shift) {
                vim_change_end_of_line(vim);
            } else {
                vim_mode(vim, VIM_CHANGE);
                vim->should_change = true;
            }
        }
        
        else if (vim->mode == VIM_NORMAL && key == SDLK_d) {
            if (shift) {
                vim_delete_end_of_line(vim);
            } else if (!control) {
                vim_mode(vim, VIM_CHANGE);
                vim->should_change = false;
            }
        }
        
        else if (vim->mode == VIM_VISUAL && key == SDLK_c && !control) {
            vim_change(vim, vim->range);
        }
        
        else if (vim->mode == VIM_VISUAL) {
            vim_range_end(vim);
        }
        
        eat(vim, key, shift);
    } else if (vim->mode == VIM_CHANGE) {
        switch (key) {
            case 'k': {
                if (vim->y > 0) {
                    vim_up(vim);
                    vim->x = 0;
                    vim_delete_line(vim, vim->y);
                    vim_delete_line(vim, vim->y);
                }
            } break;
            case 'j': {
                if (vim->y < vim->line_count-1) {
                    vim->x = 0;
                    vim_delete_line(vim, vim->y);
                    vim_delete_line(vim, vim->y);
                }
            } break;
            case 'l': {
                vim_delete_char(vim);
                vim_delete_char(vim);
            } break;
            case 'h': {
                vim_delete_char(vim);
                vim_left(vim);
                vim_delete_char(vim);
            } break;
            
            case 'w': {
                vim_range_start(vim, false);
                vim_move_forward_word(vim);
                vim_left(vim);
                vim_range_end(vim);
                vim_delete_range(vim, vim->range);
            } break;
            
            case 'e': {
                vim_range_start(vim, false);
                vim_move_end_word(vim);
                vim_range_end(vim);
                vim_delete_range(vim, vim->range);
            } break;
            
            case SDLK_LEFTBRACKET: {
                vim_range_start(vim, true);
                vim_move_up_to_empty_line(vim);
                vim_down(vim);
                vim_range_end(vim);
                vim_delete_range(vim, vim->range);
            } break;
            case SDLK_RIGHTBRACKET: {
                vim_range_start(vim, true);
                vim_move_down_to_empty_line(vim);
                vim_up(vim);
                vim_range_end(vim);
                vim_delete_range(vim, vim->range);
            } break;
        }
        
        if (vim->should_change) {
            vim_mode(vim, VIM_INSERT);
        } else {
            vim_mode(vim, VIM_NORMAL);
        }
        
        eat(vim, key, shift);
    }
    
    if (vim->mode == VIM_COMMAND || vim->mode == VIM_SEARCH) {
        if (key == SDLK_ESCAPE || key == SDLK_c && control) {
            vim_mode(vim, VIM_NORMAL);
        }
        
        if (key == SDLK_BACKSPACE) {
            vim_backspace_command_char(vim);
        }
        
        if (key == SDLK_RETURN) {
            vim_execute_command(vim);
            vim_mode(vim, VIM_NORMAL);
        }
    }
    
    vim_set_yoff(vim);
}