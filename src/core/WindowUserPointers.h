#ifndef GL_CRAFT_WINDOWUSERPOINTERS_H
#define GL_CRAFT_WINDOWUSERPOINTERS_H

class Camera;
class PostProcessingMesh;

struct WindowUserPointers {
    Camera* camera;
    PostProcessingMesh* postProcessingMesh;
};

#endif //GL_CRAFT_WINDOWUSERPOINTERS_H