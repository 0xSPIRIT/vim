typedef struct Save_State
{
    String *lines;
    u64 line_count;
    
    struct Save_State *next;
} Save_State;

void vim_save_state(Vim_Instance *vim) {
    if (!vim->state_tail) {
        vim->state_tail = allocate(sizeof(Save_State));
    } else {
        vim->state_tail->next = allocate(sizeof(Save_State));
        vim->state_tail = vim->state_tail->next;
    }
    
    Save_State *st = vim->state_tail;
    
    st->lines = allocate(vim->line_count * sizeof(String));
    st->line_count = vim->line_count;
    
    for (int i = 0; i < vim->line_count; i++) {
        st->lines[i] = string_duplicate(vim->lines[i]);
    }
}

void vim_load_state(Vim_Instance *vim, Save_State *state) {
    assert(state);
    assert(!vim->lines); // Lines must be deallocated first!
    
    int x = vim->x;
    int y = vim->y;
    
    vim->lines = allocate(state->line_count * sizeof(String));
    vim->line_count = state->line_count;
    
    for (int i = 0; i < state->line_count; i++) {
        vim->lines[i] = string_duplicate(state->lines[i]);
    }
    
    vim->x = x;
    vim->y = y;
}