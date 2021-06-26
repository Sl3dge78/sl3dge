
/*
 === TODO
 ===
 CRITICAL

 MAJOR

 BACKLOG

 IMPROVEMENTS

*/

#include <sl3dge/sl3dge.h>
#include "vulkan_types.h"
#include "vulkan_pipeline.cpp"

/*
internal void CreateSceneDescriptorSet(Renderer *context,
                                       Scene *scene,
                                       VkDescriptorSetLayout *set_layout,
                                       VkDescriptorSet *descriptor_set) {
}

internal void BuildLayout(VkDevice device,
                          const u32 set_layout_count,
                          VkDescriptorSetLayout *set_layouts,
                          VkPipelineLayout *layout) {
}

internal void VulkanUpdateDescriptors(Renderer *context, GameData *game_data) {

}

internal void BuildScene(Renderer *context, Scene *scene) {
    SDL_Log("Creating Descriptors...");
    scene->descriptor_set_count = 1;
    scene->set_layouts =
        (VkDescriptorSetLayout *)calloc(scene->descriptor_set_count, sizeof(VkDescriptorSetLayout));
    scene->descriptor_sets =
        (VkDescriptorSet *)calloc(scene->descriptor_set_count, sizeof(VkDescriptorSet));

    CreateSceneDescriptorSet(context, scene, &scene->set_layouts[0], &scene->descriptor_sets[0]);
}
/*
DLL_EXPORT Scene *VulkanCreateScene(Renderer *context) {
    Scene *scene = (Scene *)malloc(sizeof(Scene));


    CreateBuffer(context->device,
                 &context->memory_properties,
                 128 * sizeof(Material),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &scene->mat_buffer);
    DEBUGNameBuffer(context->device, &scene->mat_buffer, "SCENE MATS");

    scene->textures = (Image *)calloc(MAX_TEXTURES, sizeof(Image));
    scene->textures_count = 0;

    // TODO TEMPORARY
    scene->meshes = (Mesh **)calloc(1, sizeof(Mesh *));
    scene->mesh_count = 0;
    SDL_Log("Loading mesh...");
    const char *scene_path = "resources/3d/Motorcycle/motorcycle.gltf";
    scene->meshes[0] = LoadMesh(scene_path, context, scene);
    scene->mesh_count++;
    scene->meshes[0]->instance_count = 1;
    scene->meshes[0]->instance_transforms = (Mat4 *)calloc(1, sizeof(Mat4));
    scene->meshes[0]->instance_transforms[0] = mat4_identity();

    BuildScene(context, scene);
    SDL_Log("Scene loaded");

    return scene;
}
*/
