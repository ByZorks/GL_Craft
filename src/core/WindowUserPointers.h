#ifndef GL_CRAFT_WINDOWUSERPOINTERS_H
#define GL_CRAFT_WINDOWUSERPOINTERS_H

class Crosshair;
class Camera;
class PostProcessingMesh;

struct WindowUserPointers {
    Camera* camera;
    PostProcessingMesh* postProcessingMesh;
    Crosshair* crosshair;
};

#endif //GL_CRAFT_WINDOWUSERPOINTERS_H