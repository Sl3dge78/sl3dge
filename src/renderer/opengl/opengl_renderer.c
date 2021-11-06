#include <GL/GL.h>

#ifdef __INTELLISENSE__
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/glext.h>

#ifndef __INTELLISENSE__
#include "opengl_functions.h"
#endif

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "renderer/opengl/opengl_win32.c"
#else
#error "Platform not implemented"
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

void APIENTRY GLMessageCallback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar *message,
                                const void *userParam) {
    if(severity == GL_DEBUG_SEVERITY_HIGH) {
        sError("GL ERROR: %s", message);
        ASSERT(0);
    } else if(severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
        sWarn("GL WARN: %s", message);
    } else {
        sTrace("GL: %s", message);
    }
}

// ---------------
// Programs & shaders

internal bool CheckShaderCompilation(u32 shader, const char *path) {
    i32 result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(!result) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        sError("Unable to compile shader: %s\n%s", path, log);
    }
    return result;
}

internal bool CheckProgramLinkStatus(u32 program, const char *path) {
    i32 result;
    glGetProgramiv(program,  GL_LINK_STATUS, &result);
    if(!result) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        sError("Unable to link program %s", path);
        sError(log);
    }
    return result;
}

internal u32 CreateSeparableProgram(PlatformAPI *platform, const char *path, GLenum type) {
    i32 size = 0;
    u32 result = 0;
    platform->ReadWholeFile(path, &size, 0);
    char *const code = sCalloc(size, sizeof(char));
    platform->ReadWholeFile(path, &size, code);
    result = glCreateShaderProgramv(type, 1, (const char* const*)&code);
    ASSERT(result);
    ASSERT(CheckProgramLinkStatus(result, path));
    sFree(code);
    return result;
}

internal u32 CreateAndCompileShader(GLenum type, const char *code, const char *path) {
    u32 result = glCreateShader(type);
    glShaderSource(result, 1, &code, NULL);
    glCompileShader(result);
    ASSERT(CheckShaderCompilation(result, path));
    return result;
}

internal u32 CreateProgram(PlatformAPI *platform, const char *name) {
    u32 result = glCreateProgram();
    char buffer[128] = {0};
    
    snprintf(buffer, 128, "resources/shaders/gl/%s.vert", name);
    
    i32 file_size = 0;
    platform->ReadWholeFile(buffer, &file_size, NULL);
    char *vtx_code = sCalloc(file_size, sizeof(char));
    platform->ReadWholeFile(buffer, &file_size, vtx_code);
    
    ASSERT(vtx_code != NULL); // Vertex shaders are mandatory
    u32 vtx_shader = CreateAndCompileShader(GL_VERTEX_SHADER, vtx_code, buffer);
    sFree(vtx_code);
    glAttachShader(result, vtx_shader);
    
    snprintf(buffer, 128, "resources/shaders/gl/%s.frag", name);
    platform->ReadWholeFile(buffer, &file_size, NULL);
    char *frag_code = sCalloc(file_size, sizeof(char));
    platform->ReadWholeFile(buffer, &file_size, frag_code);
    
    u32 frag_shader = 0;
    if(frag_code == NULL) {
        sLog("No fragment shader for %s", name);
    } else {
        u32 frag_shader = CreateAndCompileShader(GL_FRAGMENT_SHADER, frag_code, buffer);
        sFree(frag_code);
        glAttachShader(result, frag_shader);
    }
    glLinkProgram(result);
    glObjectLabel(GL_PROGRAM, result, -1, name);
    glDeleteShader(vtx_shader);
    if(frag_code != NULL) {
        glDeleteShader(frag_shader);
    }
    return result;
}
// ---------------
// Shadowmap

internal void CreateShadowmapRenderPass(ShadowmapRenderPass *pass) {
    
    pass->size = 2048;
    
    // Static
    glGenProgramPipelines(1, &pass->pipeline);
    glBindProgramPipeline(pass->pipeline);
    glObjectLabel(GL_PROGRAM_PIPELINE, pass->pipeline, -1, "Shadowmap Pipeline");
    
    glGenTextures(1, &pass->texture);
    glBindTexture(GL_TEXTURE_2D, pass->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, pass->size, pass->size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenFramebuffers(1, &pass->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glObjectLabel(GL_FRAMEBUFFER, pass->framebuffer, -1, "Shadowmap framebuffer");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->texture, 0);
}

internal void DestroyShadowmapRenderPass(ShadowmapRenderPass *pass) {
    glDeleteProgramPipelines(1, &pass->pipeline);
    glDeleteFramebuffers(1, &pass->framebuffer);
    glDeleteTextures(1, &pass->texture);
}

internal void BeginShadowmapRenderPass(ShadowmapRenderPass *pass) {
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, pass->size, pass->size);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(0);
    glBindProgramPipeline(pass->pipeline);
    glCullFace(GL_BACK); 
}

// -------------
// Color

internal void CreateColorRenderPass(const u32 width, const u32 height, ColorRenderPass *pass, const u32 fragment) {
    
    pass->width = width;
    pass->height = height;
    
    // Static
    glGenProgramPipelines(1, &pass->pipeline);
    glBindProgramPipeline(pass->pipeline);
    glUseProgramStages(pass->pipeline, GL_FRAGMENT_SHADER_BIT, fragment);
    
    glObjectLabel(GL_PROGRAM_PIPELINE, pass->pipeline, -1, "ColorPass Pipeline");
    
    glGenFramebuffers(1, &pass->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glObjectLabel(GL_FRAMEBUFFER, pass->framebuffer, -1, "Color framebuffer");
    
    // Color attachment
    glGenTextures(1, &pass->render_target);
    glBindTexture(GL_TEXTURE_2D, pass->render_target);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->render_target, 0);
    
    // Depth attachment
    glGenTextures(1, &pass->depthmap);
    glBindTexture(GL_TEXTURE_2D, pass->depthmap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->depthmap, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

internal void DestroyColorRenderPass(ColorRenderPass *pass) {
    glDeleteFramebuffers(1, &pass->framebuffer);
    glDeleteTextures(1, &pass->render_target);
    glDeleteProgramPipelines(1, &pass->pipeline);
    //glDeleteProgramPipelines(1, &pass->skinned_pipeline);
    glDeleteTextures(1, &pass->depthmap);
}

internal void BeginColorRenderPass(OpenGLRenderer *renderer, ColorRenderPass *pass) {
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, pass->width, pass->height);
    
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glClearColor(0.43f, 0.77f, 0.91f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    
    // Shadowmap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_pass.texture);
    glUseProgram(0);
    glBindProgramPipeline(pass->pipeline);
}

// --------------
// Volumetric

internal void CreateVolumetricRenderPass(PlatformAPI *platform_api, VolumetricRenderPass *pass) {
    pass->program = CreateProgram(platform_api, "volumetric");
    
    glUseProgram(pass->program);
    glUniform1i(glGetUniformLocation(pass->program, "shadow_map"), 0);
    glUniform1i(glGetUniformLocation(pass->program, "depth_map"), 1);
    glUniform1i(glGetUniformLocation(pass->program, "screen_texture"), 2);
}

internal void DestroyVolumetricRenderPass(VolumetricRenderPass *pass) {
    glDeleteProgram(pass->program);
}

internal void BeginVolumetricRenderPass(Renderer *renderer, VolumetricRenderPass *pass) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(pass->program);
    glUniformMatrix4fv(glGetUniformLocation(pass->program, "cam_view"), 1, GL_FALSE, renderer->camera_view);
    glUniformMatrix4fv(glGetUniformLocation(pass->program, "proj_inverse"), 1, GL_FALSE, renderer->camera_proj_inverse);
    glUniformMatrix4fv(glGetUniformLocation(pass->program, "view_inverse"), 1, GL_FALSE, renderer->camera_view_inverse);
    glUniform3f(glGetUniformLocation(pass->program, "view_pos"), renderer->camera_pos.x, renderer->camera_pos.y, renderer->camera_pos.z);
    glUniform3f(glGetUniformLocation(pass->program, "light_dir"), renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z);
    glUniformMatrix4fv(glGetUniformLocation(pass->program, "light_matrix"), 1, GL_FALSE, renderer->light_matrix);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->backend->shadowmap_pass.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->backend->color_pass.depthmap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->backend->color_pass.render_target);
}

// ---------------
// Renderer

void BackendRendererInit(OpenGLRenderer *renderer, PlatformAPI *platform_api, PlatformWindow *window) {
    PlatformCreateorUpdateOpenGLContext(renderer, window);
    GLLoadFunctions();
    
    // Global init
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);
    glDisable(GL_CULL_FACE);
    
    // Shaders
    renderer->static_mesh_vtx_shader = CreateSeparableProgram(platform_api,"resources/shaders/gl/static_mesh.vert", GL_VERTEX_SHADER);
    glObjectLabel(GL_PROGRAM, renderer->static_mesh_vtx_shader, -1, "Static Mesh Vertex Shader");
    
    renderer->skinned_mesh_vtx_shader = CreateSeparableProgram(platform_api,"resources/shaders/gl/skinned_mesh.vert", GL_VERTEX_SHADER);
    glObjectLabel(GL_PROGRAM, renderer->skinned_mesh_vtx_shader, -1, "Skinned Mesh Vertex Shader");
    
    renderer->color_fragment_shader = CreateSeparableProgram(platform_api, "resources/shaders/gl/color.frag", GL_FRAGMENT_SHADER);
    glObjectLabel(GL_PROGRAM, renderer->color_fragment_shader, -1, "Color Fragment Shader");
    
    glProgramUniform1i(renderer->color_fragment_shader, glGetUniformLocation(renderer->color_fragment_shader,"shadow_map"), 0);
    glProgramUniform1i(renderer->color_fragment_shader, glGetUniformLocation(renderer->color_fragment_shader,"diffuse"), 1);
    
    
    // Render passes
    CreateShadowmapRenderPass(&renderer->shadowmap_pass);
    CreateColorRenderPass(window->w, window->h, &renderer->color_pass, renderer->color_fragment_shader);
    CreateVolumetricRenderPass(platform_api, &renderer->vol_pass);
    
    // Screen quad
    glGenVertexArrays(1, &renderer->screen_quad);
    glBindVertexArray(renderer->screen_quad);
    glGenBuffers(1, &renderer->screen_quad_vbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->screen_quad_vbuffer);
    const f32 quad[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    
    { // UI
        // Load font
        unsigned char *ttf_buffer = sCalloc(1 << 20, sizeof(unsigned char));
        FILE *f = fopen("resources/font/LiberationMono-Regular.ttf", "rb");
        ASSERT(f);
        fread(ttf_buffer, 1, 1 << 20, f);
        fclose(f);
        
        unsigned char *temp_bmp = sCalloc(512 * 512, sizeof(unsigned char));
        renderer->char_data = sCalloc(96, sizeof(stbtt_bakedchar));
        
        stbtt_BakeFontBitmap(ttf_buffer, 0, 20.0f, temp_bmp, 512, 512, 32, 96, renderer->char_data);
        
        sFree(ttf_buffer);
        glGenTextures(1, &renderer->glyphs_texture);
        glBindTexture(GL_TEXTURE_2D, renderer->glyphs_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bmp);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        sFree(temp_bmp);
        
        // Init glyph array
        glGenVertexArrays(1, &renderer->ui_vertex_array);
        glBindVertexArray(renderer->ui_vertex_array);
        glGenBuffers(1, &renderer->ui_vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->ui_vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
        glEnableVertexAttribArray(1);
        
        // Load ui shader
        renderer->ui_program = CreateProgram(platform_api, "ui");
        glUseProgram(renderer->ui_program);
        glUniform1i(glGetUniformLocation(renderer->ui_program, "alpha_map"), 0);
        glUniform1i(glGetUniformLocation(renderer->ui_program, "color_texture"), 1);
        
        // Load white texture
        glGenTextures(1, &renderer->white_texture);
        glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
        u8 *white = sCalloc(4 * 4 * 3, sizeof(u8));
        memset(white, 255, 4 * 4 * 3 * sizeof(u8));
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        sFree(white);
    }
    
    renderer->line_program = CreateProgram(platform_api, "debug");
}

void BackendRendererDestroy(OpenGLRenderer *renderer) {
    
    sFree(renderer->char_data);
    glDeleteTextures(1, &renderer->white_texture);
    glDeleteTextures(1, &renderer->glyphs_texture);
    
    // Render passes
    DestroyShadowmapRenderPass(&renderer->shadowmap_pass);
    DestroyColorRenderPass(&renderer->color_pass);
    DestroyVolumetricRenderPass(&renderer->vol_pass);
    
    // Shaders
    glDeleteProgram(renderer->static_mesh_vtx_shader);
    glDeleteProgram(renderer->skinned_mesh_vtx_shader);
    glDeleteProgram(renderer->color_fragment_shader);
    
    // Screen quad
    glDeleteVertexArrays(1, &renderer->screen_quad_vbuffer);
    glDeleteBuffers(1, &renderer->screen_quad);
}

// -------------
// Drawing

internal void DrawMesh(const u32 pipeline, const u32 vtx_shader, const u32 frag_shader, const Mesh *mesh, const Mat4 xform, const Vec3 color) {
    glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vtx_shader);
    glProgramUniform3f(frag_shader, glGetUniformLocation(frag_shader, "diffuse_color"), color.x, color.y, color.z); 
    
    glProgramUniformMatrix4fv(vtx_shader, glGetUniformLocation(vtx_shader, "transform"), 1, GL_FALSE, xform);
    
    glBindVertexArray(mesh->vertex_array);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
}


internal void DrawScene(OpenGLRenderer *renderer, PushBuffer *pushb, const u32 pipeline) {
    glActiveTexture(GL_TEXTURE1);
    
    if(pushb->size == 0)
        return;
    
    for(u32 address = 0; address < pushb->size;) {
        PushBufferEntryType *type = (PushBufferEntryType *)(pushb->buf + address);
        switch(*type) {
            case PushBufferEntryType_Mesh: {
                PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(pushb->buf + address);
                
                Mat4 mat;
                transform_to_mat4(entry->transform, &mat);
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                
                DrawMesh(pipeline, renderer->static_mesh_vtx_shader, renderer->color_fragment_shader, entry->mesh, mat, entry->diffuse_color);
                
                address += sizeof(PushBufferEntryMesh);
            } break;
            case PushBufferEntryType_Skin: {
                PushBufferEntrySkin *entry = (PushBufferEntrySkin *)(pushb->buf + address);
                
                Mat4 mesh_transform;
                transform_to_mat4(entry->transform, &mesh_transform);
                
                // Skin calc
                SkinHandle skin = entry->skin;
                
                // Calculate bone xforms
                Mat4 *joint_mats = sCalloc(skin->joint_count, sizeof(Mat4));
                
                Mat4 mesh_inverse;
                mat4_inverse(mesh_transform, mesh_inverse);
                
                u32 root = 0;
                for(u32 i = 0; i < skin->joint_count; i++){
                    if(skin->joint_parents[i] == -1) {
                        root = i;
                        break;
                    }
                }
                
                Mat4 tmp;
                transform_to_mat4(&skin->joints[root], &tmp);
                mat4_mul(mesh_transform, tmp, skin->global_joint_mats[root]);
                CalcChildXform(root, skin);
                
                for(u32 i = 0; i < skin->joint_count; i++) {
                    mat4_mul(skin->global_joint_mats[i], skin->inverse_bind_matrices[i], tmp); // Inverse Bind Matrix
                    mat4_mul(mesh_inverse, tmp, joint_mats[i]);
                }
                
                glProgramUniformMatrix4fv(renderer->skinned_mesh_vtx_shader, glGetUniformLocation(renderer->skinned_mesh_vtx_shader, "joint_matrices"), skin->joint_count, GL_FALSE, (f32*)joint_mats);
                
                // Mesh
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                DrawMesh(pipeline, renderer->skinned_mesh_vtx_shader, renderer->color_fragment_shader, entry->mesh, mesh_transform, entry->diffuse_color);
                sFree(joint_mats);
                
                address += sizeof(PushBufferEntrySkin);
            } break;
            default : {
                ASSERT(0);
            }
        };
        
        
    }
}

internal void DrawScreenQuad(OpenGLRenderer *renderer) {
    glBindVertexArray(renderer->screen_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

internal void DrawUI(Renderer *frontend, PushBuffer *push_buffer) {
    OpenGLRenderer *renderer = frontend->backend;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(renderer->ui_program);
    Mat4 ortho; 
    mat4_ortho_gl(0, frontend->height, 0, frontend->width, -1, 1, ortho);
    glUniformMatrix4fv(glGetUniformLocation(renderer->ui_program, "proj"), 1, GL_FALSE, ortho);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer->ui_vertex_buffer);
    glBindVertexArray(renderer->ui_vertex_array);
    const u32 color_uniform = glGetUniformLocation(renderer->ui_program, "color");
    
    for(u32 address = 0; address < push_buffer->size;) {
        PushBufferEntryType *type = (PushBufferEntryType *)(push_buffer->buf + address);
        
        switch(*type) {
            case PushBufferEntryType_UIQuad: {
                PushBufferEntryUIQuad *entry = (PushBufferEntryUIQuad *)(push_buffer->buf + address);
                
                const f32 vtx[] = {
                    entry->l,
                    entry->t,
                    0.0f,
                    1.0f, // UL
                    entry->r,
                    entry->t,
                    1.0f,
                    1.0f, // UR
                    entry->l,
                    entry->b,
                    0.0f,
                    0.0f, // LL
                    entry->r,
                    entry->b,
                    1.0f,
                    0.0f, // LR
                };
                glUniform4f(
                            color_uniform, entry->colour.x, entry->colour.y, entry->colour.z, entry->colour.w);
                glActiveTexture(GL_TEXTURE0); // Alpha map
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                glActiveTexture(GL_TEXTURE1); // Color map
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                
                glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), &vtx, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                address += sizeof(PushBufferEntryUIQuad);
                continue;
            }
            case PushBufferEntryType_Text: {
                glActiveTexture(GL_TEXTURE0); // Alpha map
                glBindTexture(GL_TEXTURE_2D, renderer->glyphs_texture);
                glActiveTexture(GL_TEXTURE1); // Color map
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                
                PushBufferEntryText *entry = (PushBufferEntryText *)(push_buffer->buf + address);
                const char *txt = entry->text;
                f32 x = (f32)entry->x;
                f32 y = (f32)entry->y;
                
                while(*txt) {
                    if(*txt >= 32 && *txt <= 127) {
                        stbtt_aligned_quad q;
                        stbtt_GetBakedQuad(renderer->char_data, 512, 512, *txt - 32, &x, &y, &q, 1);
                        
                        f32 vtx[] = {
                            q.x0,
                            q.y0,
                            q.s0,
                            q.t0, // 0, 0
                            q.x1,
                            q.y0,
                            q.s1,
                            q.t0, // 1, 0
                            q.x0,
                            q.y1,
                            q.s0,
                            q.t1, // 0, 1
                            q.x1,
                            q.y1,
                            q.s1,
                            q.t1, // 1, 1
                        };
                        
                        glUniform4f(color_uniform,
                                    entry->colour.x,
                                    entry->colour.y,
                                    entry->colour.z,
                                    entry->colour.w);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx, GL_DYNAMIC_DRAW);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    }
                    txt++;
                }
                sFree(entry->text);
                address += sizeof(PushBufferEntryText);
                continue;
            } break;
            case PushBufferEntryType_Texture: {
                PushBufferEntryTexture *entry = (PushBufferEntryTexture *)(push_buffer->buf + address);
                
                const f32 vtx[] = {
                    entry->l,
                    entry->t,
                    0.0f,
                    1.0f, // UL
                    entry->r,
                    entry->t,
                    1.0f,
                    1.0f, // UR
                    entry->l,
                    entry->b,
                    0.0f,
                    0.0f, // LL
                    entry->r,
                    entry->b,
                    1.0f,
                    0.0f, // LR
                };
                glUniform4f(
                            color_uniform, 1.0f, 1.0f, 1.0f, 1.0f);
                glActiveTexture(GL_TEXTURE0); // Alpha map
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                glActiveTexture(GL_TEXTURE1); // Color map
                glBindTexture(GL_TEXTURE_2D, entry->texture);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), &vtx, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                
                glActiveTexture(GL_TEXTURE1); // Color map
                glBindTexture(GL_TEXTURE_2D, renderer->white_texture);
                
                address += sizeof(PushBufferEntryTexture);
                continue;
            }
            default: {
                ASSERT(0);
            }
        }
    }
}

internal void DrawDebug(OpenGLRenderer *renderer, PushBuffer *pushb, Mat4 camera_proj, Mat4 camera_view) {
    
    glUseProgram(renderer->line_program);
    
    glUniformMatrix4fv(glGetUniformLocation(renderer->line_program, "projection"), 1, GL_FALSE, camera_proj);
    glUniformMatrix4fv(glGetUniformLocation(renderer->line_program, "view"), 1, GL_FALSE, camera_view);
    
    if(pushb->size == 0)
        return;
    
    for(u32 address = 0; address < pushb->size;) {
        PushBufferEntryType *type = (PushBufferEntryType *)(pushb->buf + address);
        
        switch (*type) {
            case PushBufferEntryType_Bone: {
                
                PushBufferEntryBone *entry = (PushBufferEntryBone *)(pushb->buf + address);
                u32 vao;
                u32 vbo;
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                
                u32 stride = sizeof(Vec3) + sizeof(Vec3);
                
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(Vec3)));
                glEnableVertexAttribArray(1);
                
                const Vec3 r = (Vec3){1.0f, 0.0F, 0.0F};
                const Vec3 g = (Vec3){0.0f, 1.0F, 0.0F};
                const Vec3 b = (Vec3){0.0f, 0.0F, 1.0F};
                
                Vec3 buf[] = {
                    entry->line[0],
                    r,
                    entry->line[1],
                    r,
                    entry->line[0],
                    g,
                    entry->line[2],
                    g,
                    entry->line[0],
                    b,
                    entry->line[3],
                    b
                };
                
                glBufferData(GL_ARRAY_BUFFER, sizeof(buf), &buf, GL_STREAM_DRAW);
                
                glDrawArrays(GL_LINES, 0, 6);
                glDeleteBuffers(1, &vbo);
                glDeleteVertexArrays(1, &vao);
                address += sizeof(PushBufferEntryBone);
                continue;
            } break;
            default :{
                ASSERT(0);
            } break;
        }
    }
}

DLL_EXPORT void RendererDrawFrame(Renderer *frontend) {
    // ------------------
    // Uniforms
    OpenGLRenderer *backend = frontend->backend;
    mat4_mul(frontend->camera_proj, frontend->camera_view, frontend->camera_vp);
    glProgramUniformMatrix4fv(backend->static_mesh_vtx_shader, glGetUniformLocation(backend->static_mesh_vtx_shader, "light_matrix"), 1, GL_FALSE, frontend->light_matrix);
    glProgramUniformMatrix4fv(backend->skinned_mesh_vtx_shader, glGetUniformLocation(backend->skinned_mesh_vtx_shader, "light_matrix"), 1, GL_FALSE, frontend->light_matrix);
    glProgramUniform3f(backend->color_fragment_shader, glGetUniformLocation(backend->color_fragment_shader, "light_dir"), frontend->light_dir.x, frontend->light_dir.y, frontend->light_dir.z); 
    
    // ------------------
    // Shadow map
    BeginShadowmapRenderPass(&backend->shadowmap_pass);
    glProgramUniformMatrix4fv(backend->static_mesh_vtx_shader, glGetUniformLocation(backend->static_mesh_vtx_shader, "vp"), 1, GL_FALSE, frontend->light_matrix);
    glProgramUniformMatrix4fv(backend->skinned_mesh_vtx_shader, glGetUniformLocation(backend->skinned_mesh_vtx_shader, "vp"), 1, GL_FALSE, frontend->light_matrix);
    DrawScene(backend, &frontend->scene_pushbuffer, backend->shadowmap_pass.pipeline);
    
    // ------------------
    // Color pass
    BeginColorRenderPass(backend, &backend->color_pass);
    glProgramUniformMatrix4fv(backend->static_mesh_vtx_shader, glGetUniformLocation(backend->static_mesh_vtx_shader, "vp"), 1, GL_FALSE, frontend->camera_vp);
    glProgramUniformMatrix4fv(backend->skinned_mesh_vtx_shader, glGetUniformLocation(backend->skinned_mesh_vtx_shader, "vp"), 1, GL_FALSE, frontend->camera_vp);
    DrawScene(backend, &frontend->scene_pushbuffer, backend->color_pass.pipeline);
    
    // ---------------
    // Post process
    BeginVolumetricRenderPass(frontend, &backend->vol_pass);
    DrawScreenQuad(backend);
    
    DrawDebug(backend, &frontend->debug_pushbuffer, frontend->camera_proj, frontend->camera_view);
    
    DrawUI(frontend, &frontend->ui_pushbuffer);
    
    PlatformSwapBuffers(frontend->window);
    
    // Clear pushbuffers
    frontend->scene_pushbuffer.size = 0; // @Optimization : Maybe we don't need to reset it each frame? do some tests
    frontend->ui_pushbuffer.size = 0;
    frontend->debug_pushbuffer.size = 0;
}

DLL_EXPORT void RendererUpdateWindow(Renderer *renderer, PlatformAPI *platform_api, const u32 width, const u32 height) {
    sLog("Update window!");
    
    renderer->width = width;
    renderer->height = height;
    
    DestroyColorRenderPass(&renderer->backend->color_pass);
    CreateColorRenderPass(width, height, &renderer->backend->color_pass, renderer->backend->color_fragment_shader);
    UpdateCameraProj(renderer);
}

/*
void RendererReloadShaders(Renderer *renderer, PlatformAPI *platform_api) {
    // TODO(Guigui): This won't work now. We need to reload the shader programs too
    DestroyShadowmapRenderPass(&renderer->backend->shadowmap_pass);
    CreateShadowmapRenderPass(&renderer->backend->shadowmap_pass);
    
    DestroyColorRenderPass(&renderer->backend->color_pass);
    CreateColorRenderPass(renderer->width, renderer->height, &renderer->backend->color_pass, renderer->backend->color_fragment_shader);
    
    DestroyVolumetricRenderPass(&renderer->backend->vol_pass);
    CreateVolumetricRenderPass(platform_api, &renderer->backend->vol_pass);
}

*/

