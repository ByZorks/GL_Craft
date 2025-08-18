#include "BlockSelector.h"

BlockSelector::BlockSelector() = default;

BlockSelector::BlockSelector(Shader *shader, HighlightedBlock *mesh) : m_shader(shader), m_mesh(mesh) {}

void BlockSelector::init(Shader *shader, HighlightedBlock *mesh) {
    m_shader = shader;
    m_mesh = mesh;
}

void BlockSelector::render(const RaycastResult &hit, const glm::mat4 &mvp) const {
    if (!hit.hitBlock) return;

    m_shader->use();
    m_shader->setUniformMat4f("u_MVP", mvp);
    m_shader->setUniform3f("u_Offset",
        static_cast<float>(hit.blockWorldPosition[0]),
        static_cast<float>(hit.blockWorldPosition[1]),
        static_cast<float>(hit.blockWorldPosition[2]));
    m_mesh->draw();
}
