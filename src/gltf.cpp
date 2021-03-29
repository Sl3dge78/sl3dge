
typedef struct GLTF_Value {
	u32 name_size;
	char *name;
    
	union {
		u32 int_value;
		float float_value;
	};
    
} GLTF_Value;

internal void GLTFReadIdentifier(char *c, GLTF_Value *value) {
	char *start = c;
    
    c++;
    
    value->name_size = 0;
	while (*c != '\"') {
		c++;
		value->name_size++;
	}
    
	value->name = (char *)calloc(value->name_size, sizeof(char));
    memcpy(value->name, start + 1, value->name_size * sizeof(char));
    
    bool do_the_parse = false;
    bool parsed = false;
    
    u32 decimal;
    
    // Get the value
    while(1) {
        
        if(parsed && !do_the_parse)
            break;
        
        switch(c) {
            case ':': {
                do_the_parse = true;
            }break;
            
            case ',': {
                do_the_parse = false;
            }break;
            
            case '0': {
                
            }
            
        }
        
        
        
        if(c ==
           
           c++
           
    }
}

internal void Win32LoadGLTF(char *path) {
	struct GLTF {
		char *content;
		u32 file_size;
	} gltf;
    
	FILE *file = fopen(path, "r");
	if (!file) {
		SDL_LogError(0, "Unable to open file");
		SDL_LogError(0, path);
	}
	// Get the size
	fseek(file, 0, SEEK_END);
	gltf.file_size = ftell(file);
	rewind(file);
    
	// Copy into result
	gltf.content = (char *)malloc(gltf.file_size);
	fread(gltf.content, 1, gltf.file_size, file);
	fclose(file);
    
	char *c = gltf.content;
	u32 depth = 0;
    
	char *current_identifier = "";
    
	while (*c != '\0') {
		if (*c == '{') {
			depth++;
		}
		if (*c == '}') {
			depth--;
		}
        
		// Read identifier
		if (*c == '\"') {
			GLTF_Value value = {};
			GLTFReadIdentifier(c, &value);
		}
        
		c++;
	}
    
	SDL_Log("%s", gltf.content);
}