#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/GL.h>

#ifdef __INTELLISENSE__
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/glext.h>

#ifndef __INTELLISENSE__
#include "opengl_functions.h"
#endif

#ifdef __WIN32__
#include "renderer/opengl/opengl_win32.c"
#else
#error "Platform not implemented"
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "utils/sl3dge.h"
#include <cgltf.h>

#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"
#include "renderer/gltf.c"
#include "renderer/opengl/mesh.c"

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

// --------------
// Mesh

// ---------------
// Shadowmap

internal void CreateShadowmapRenderPass(ShadowmapRenderPass *pass) {
    
    // Static
    glGenProgramPipelines(1, &pass->pipeline);
    glBindProgramPipeline(pass->pipeline);
    glObjectLabel(GL_PROGRAM_PIPELINE, pass->pipeline, -1, "Shadowmap Pipeline");
    
    glGenTextures(1, &pass->texture);
    glBindTexture(GL_TEXTURE_2D, pass->texture);
    glTexImage2D(
                 GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    Vec4 border_color = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (f32 *)&border_color);
    
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
    glViewport(0, 0, 2048, 2048);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(0);
    glBindProgramPipeline(pass->pipeline);
}

// -------------
// Color

internal void CreateColorRenderPass(const u32 width, const u32 height, ColorRenderPass *pass, const u32 fragment) {
    
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
    glFramebufferTexture2D(
                           GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->render_target, 0);
    
    // Depth attachment
    glGenTextures(1, &pass->depthmap);
    glBindTexture(GL_TEXTURE_2D, pass->depthmap);
    glTexImage2D(
                 GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

internal void BeginColorRenderPass(Renderer *renderer, ColorRenderPass *pass) {
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, renderer->width, renderer->height);
    
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
    glUniformMatrix4fv(
                       glGetUniformLocation(pass->program, "cam_view"), 1, GL_FALSE, renderer->camera_view);
    
    glUniformMatrix4fv(glGetUniformLocation(renderer->vol_pass.program, "proj_inverse"), 1, GL_FALSE, renderer->camera_proj_inverse);
    
    glUniformMatrix4fv(glGetUniformLocation(pass->program, "view_inverse"),
                       1,
                       GL_FALSE,
                       renderer->camera_view_inverse);
    glUniform3f(glGetUniformLocation(pass->program, "view_pos"),
                renderer->camera_pos.x,
                renderer->camera_pos.y,
                renderer->camera_pos.z);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->shadowmap_pass.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->color_pass.depthmap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->color_pass.render_target);
}

// ---------------
// Renderer

void UpdateCameraProj(Renderer *renderer) {
    mat4_perspective_gl(90.0f, (f32)renderer->width / (f32)renderer->height, 0.1f, 100000.0f, renderer->camera_proj);
    mat4_inverse(renderer->camera_proj, renderer->camera_proj_inverse);
}

DLL_EXPORT u32 GetRendererSize() {
    u32 result = sizeof(Renderer);
    return result;
}

DLL_EXPORT void RendererInit(Renderer *renderer, PlatformAPI *platform_api, PlatformWindow *window) {
    renderer->window = window;
    PlatformCreateorUpdateOpenGLContext(renderer, window);
    GLLoadFunctions();
    
    // Global init
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);
    glDisable(GL_CULL_FACE);
    
    // Arrays
    renderer->mesh_count = 0;
    renderer->mesh_capacity = 8;
    renderer->meshes = sCalloc(renderer->mesh_capacity, sizeof(Mesh));
    
    renderer->skinned_mesh_count = 0;
    renderer->skinned_mesh_capacity = 8;
    renderer->skinned_meshes = sCalloc(renderer->mesh_capacity, sizeof(SkinnedMesh));
    
    renderer->transform_count = 0;
    renderer->transform_capacity = 16;
    renderer->transforms = sCalloc(renderer->transform_capacity, sizeof(Transform));
    
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
        // Init push buffer
        renderer->ui_pushbuffer.size = 0;
        renderer->ui_pushbuffer.max_size = sizeof(PushBufferEntryText) * 256;
        renderer->ui_pushbuffer.buf = sCalloc(renderer->ui_pushbuffer.max_size, 1);
        
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
    
    { // Scene
        renderer->scene_pushbuffer.size = 0;
        renderer->scene_pushbuffer.max_size = sizeof(PushBufferEntryMesh) * 70;
        renderer->scene_pushbuffer.buf = sCalloc(renderer->scene_pushbuffer.max_size, 1);
    }
    
    { // Debug 
        renderer->line_program = CreateProgram(platform_api, "debug");
        renderer->debug_pushbuffer.size = 0;
        renderer->debug_pushbuffer.max_size = sizeof(PushBufferEntryBone) * 20;
        renderer->debug_pushbuffer.buf = sCalloc(renderer->debug_pushbuffer.max_size, 1);
        
    }
    UpdateCameraProj(renderer);
}

DLL_EXPORT void RendererDestroy(Renderer *renderer) {
    
    // UI
    sFree(renderer->ui_pushbuffer.buf);
    sFree(renderer->scene_pushbuffer.buf);
    sFree(renderer->debug_pushbuffer.buf);
    
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
    
    // Meshes
    for(u32 i = 0; i < renderer->mesh_count; i++) {
        RendererDestroyMesh(renderer, &renderer->meshes[i]);
    }
    sFree(renderer->meshes);
    for(u32 i = 0; i < renderer->skinned_mesh_count; i++) {
        RendererDestroySkinnedMesh(renderer, &renderer->skinned_meshes[i]);
    }
    sFree(renderer->skinned_meshes);
    
    sFree(renderer->transforms);
}

// -------------
// Drawing

internal void DrawScene(Renderer *renderer, const u32 pipeline) {
    glActiveTexture(GL_TEXTURE1);
    
    PushBuffer *pushb = &renderer->scene_pushbuffer;
    if(pushb->size == 0)
        return;
    
    for(u32 address = 0; address < pushb->size;) {
        PushBufferEntryType *type = (PushBufferEntryType *)(pushb->buf + address);
        switch(*type) {
            case PushBufferEntryType_Mesh: {
                PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(pushb->buf + address);
                
                Mesh *mesh = &renderer->meshes[entry->mesh_handle];
                glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, renderer->static_mesh_vtx_shader);
                glBindTexture(GL_TEXTURE_2D, mesh->diffuse_texture);
                glProgramUniform3f(renderer->color_fragment_shader, glGetUniformLocation(renderer->color_fragment_shader, "diffuse_color"), entry->diffuse_color.x, entry->diffuse_color.y, entry->diffuse_color.z); 
                
                glBindVertexArray(mesh->vertex_array);
                
                Mat4 mat;
                TransformToMat4(entry->transform, &mat);
                glProgramUniformMatrix4fv(renderer->static_mesh_vtx_shader, glGetUniformLocation(renderer->static_mesh_vtx_shader, "transform"), 1, GL_FALSE, mat);
                glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
                
                address += sizeof(PushBufferEntryMesh);
            } break;
            case PushBufferEntryType_SkinnedMesh: {
                PushBufferEntrySkinnedMesh *entry = (PushBufferEntrySkinnedMesh *)(pushb->buf + address);
                SkinnedMeshHandle mesh = entry->mesh_handle;
                
                glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, renderer->skinned_mesh_vtx_shader);
                glBindTexture(GL_TEXTURE_2D, mesh->mesh.diffuse_texture);
                glProgramUniform3f(renderer->color_fragment_shader, glGetUniformLocation(renderer->color_fragment_shader, "diffuse_color"), entry->diffuse_color.x, entry->diffuse_color.y, entry->diffuse_color.z); 
                
                Mat4 mesh_transform;
                TransformToMat4(entry->transform, &mesh_transform);
                glProgramUniformMatrix4fv(renderer->skinned_mesh_vtx_shader, glGetUniformLocation(renderer->skinned_mesh_vtx_shader, "transform"), 1, GL_FALSE, mesh_transform);
                
                // Calculate bone xforms
                Mat4 *joint_mats = sCalloc(mesh->joint_count, sizeof(Mat4));
                
                Mat4 mesh_inverse;
                mat4_inverse(mesh_transform, mesh_inverse);
                for(u32 i = 0; i < mesh->joint_count; i++) {
                    
                    Mat4 *parent_global_xform;
                    if(mesh->joint_parents[i] == -1) { // No parent
                        parent_global_xform = &mesh_transform;
                    } else if (mesh->joint_parents[i] < i) { // We already calculated the global xform of the parent
                        parent_global_xform = &mesh->global_joint_mats[mesh->joint_parents[i]];
                    } else { // We need to calculate it
                        ASSERT(0); // @TODO
                    }
                    Mat4 tmp;
                    TransformToMat4(&mesh->joints[i], &tmp);
                    mat4_mul(*parent_global_xform, tmp, mesh->global_joint_mats[i]); // Global Transform
                    mat4_mul(mesh->global_joint_mats[i],mesh->inverse_bind_matrices[i], tmp); // Inverse Bind Matrix
                    mat4_mul(mesh_inverse, tmp, joint_mats[i]);
                }
                glProgramUniformMatrix4fv(renderer->skinned_mesh_vtx_shader, glGetUniformLocation(renderer->skinned_mesh_vtx_shader, "joint_matrices"), mesh->joint_count, GL_FALSE, (f32*)joint_mats);
                
                glBindVertexArray(mesh->mesh.vertex_array);
                glDrawElements(GL_TRIANGLES, mesh->mesh.index_count, GL_UNSIGNED_INT, 0);
                sFree(joint_mats);
                
                address += sizeof(PushBufferEntrySkinnedMesh);
            } break;
            default : {
                ASSERT(0);
            }
        };
        
        
    }
}

internal void DrawScreenQuad(Renderer *renderer) {
    glBindVertexArray(renderer->screen_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

internal void DrawUI(Renderer *renderer, PushBuffer *push_buffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(renderer->ui_program);
    Mat4 ortho; 
    mat4_ortho_gl(0, renderer->height, 0, renderer->width, -1, 1, ortho);
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

internal void DrawDebug(Renderer *renderer, PushBuffer *push_buffer) {
    
    glUseProgram(renderer->line_program);
    
    glUniformMatrix4fv(glGetUniformLocation(renderer->line_program, "projection"), 1, GL_FALSE, renderer->camera_proj);
    glUniformMatrix4fv(glGetUniformLocation(renderer->line_program, "view"), 1, GL_FALSE, renderer->camera_view);
    
    PushBuffer *pushb = &renderer->debug_pushbuffer;
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

DLL_EXPORT void RendererDrawFrame(Renderer *renderer) {
    // ------------------
    // Uniforms
    mat4_mul(renderer->camera_proj, renderer->camera_view, renderer->camera_vp);
    glProgramUniformMatrix4fv(renderer->static_mesh_vtx_shader, glGetUniformLocation(renderer->static_mesh_vtx_shader, "light_matrix"), 1, GL_FALSE, renderer->light_matrix);
    glProgramUniformMatrix4fv(renderer->skinned_mesh_vtx_shader, glGetUniformLocation(renderer->skinned_mesh_vtx_shader, "light_matrix"), 1, GL_FALSE, renderer->light_matrix);
    glProgramUniform3f(renderer->color_fragment_shader, glGetUniformLocation(renderer->color_fragment_shader, "light_dir"),renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z); 
    
    // ------------------
    // Shadow map
    BeginShadowmapRenderPass(&renderer->shadowmap_pass);
    glProgramUniformMatrix4fv(renderer->static_mesh_vtx_shader, glGetUniformLocation(renderer->static_mesh_vtx_shader, "vp"), 1, GL_FALSE, renderer->light_matrix);
    glProgramUniformMatrix4fv(renderer->skinned_mesh_vtx_shader, glGetUniformLocation(renderer->skinned_mesh_vtx_shader, "vp"), 1, GL_FALSE, renderer->light_matrix);
    DrawScene(renderer, renderer->shadowmap_pass.pipeline);
    
    // ------------------
    // Color pass
    BeginColorRenderPass(renderer, &renderer->color_pass);
    glProgramUniformMatrix4fv(renderer->static_mesh_vtx_shader, glGetUniformLocation(renderer->static_mesh_vtx_shader, "vp"), 1, GL_FALSE, renderer->camera_vp);
    glProgramUniformMatrix4fv(renderer->skinned_mesh_vtx_shader, glGetUniformLocation(renderer->skinned_mesh_vtx_shader, "vp"), 1, GL_FALSE, renderer->camera_vp);
    DrawScene(renderer, renderer->color_pass.pipeline);
    
    // ---------------
    // Post process
    BeginVolumetricRenderPass(renderer, &renderer->vol_pass);
    DrawScreenQuad(renderer);
    
    DrawDebug(renderer, &renderer->debug_pushbuffer);
    
    DrawUI(renderer, &renderer->ui_pushbuffer);
    
    PlatformSwapBuffers(renderer);
    
    // Clear pushbuffers
    renderer->scene_pushbuffer.size = 0; // @Optimization : Maybe we don't need to reset it each frame? do some tests
    renderer->ui_pushbuffer.size = 0;
    renderer->debug_pushbuffer.size = 0;
}

DLL_EXPORT void RendererUpdateWindow(Renderer *renderer, PlatformAPI *platform_api, const u32 width, const u32 height) {
    sLog("Update window!");
    
    renderer->width = width;
    renderer->height = height;
    
    DestroyColorRenderPass(&renderer->color_pass);
    CreateColorRenderPass(width, height, &renderer->color_pass, renderer->color_fragment_shader);
    UpdateCameraProj(renderer);
}

void RendererReloadShaders(Renderer *renderer, PlatformAPI *platform_api) {
    // TODO(Guigui): This won't work now. We need to reload the shader programs too
    DestroyShadowmapRenderPass(&renderer->shadowmap_pass);
    CreateShadowmapRenderPass(&renderer->shadowmap_pass);
    
    DestroyColorRenderPass(&renderer->color_pass);
    CreateColorRenderPass(renderer->width, renderer->height, &renderer->color_pass, renderer->color_fragment_shader);
    
    DestroyVolumetricRenderPass(&renderer->vol_pass);
    CreateVolumetricRenderPass(platform_api, &renderer->vol_pass);
}

void RendererSetCamera(Renderer *renderer, const Mat4 view, const Vec3 pos) {
    renderer->camera_pos = pos;
    memcpy(renderer->camera_view, view, 16 * sizeof(f32));
    mat4_inverse(view, renderer->camera_view_inverse);
    
    // Update uniforms
    // The uniform for the mesh vtx shader is updated each frame in RendererDrawFrame
    
    // Volumetric pass
    glUseProgram(renderer->vol_pass.program);
    u32 loc = glGetUniformLocation(renderer->vol_pass.program, "view_inverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->camera_view_inverse);
    loc = glGetUniformLocation(renderer->vol_pass.program, "view_pos");
    glUniform3f(loc, renderer->camera_pos.x, renderer->camera_pos.y, renderer->camera_pos.z);
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
    Mat4 ortho;
    mat4_ortho_zoom_gl(1.0f, 10.0f, -10.0f, 10.0f, ortho);
    Mat4 look;
    mat4_look_at(vec3_add(direction, renderer->camera_pos),
                 renderer->camera_pos,
                 (Vec3){0.0f, 1.0f, 0.0f}, look);
    mat4_mul(ortho, look, renderer->light_matrix);
    renderer->light_dir = direction;
    
    // Update uniforms
    // Uniforms for vertex and fragment shaders are updated each frame in RendererDrawFrame
    
    // Volumetric
    glUseProgram(renderer->vol_pass.program);
    u32 loc = glGetUniformLocation(renderer->vol_pass.program, "light_dir");
    glUniform3f(loc, renderer->light_dir.x, renderer->light_dir.y, renderer->light_dir.z);
    loc = glGetUniformLocation(renderer->vol_pass.program, "light_matrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, renderer->light_matrix);
}