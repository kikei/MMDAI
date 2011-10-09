/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2011  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                2010-2011  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#include <vpvl/vpvl.h>
#include <vpvl/gl2/Renderer.h>

#include <btBulletDynamicsCommon.h>

#ifdef VPVL_LINK_ASSIMP
#include <aiScene.h>
#include <map>
#endif

namespace vpvl
{
namespace gl2
{
class ShaderProgram
{
public:
    ShaderProgram(IDelegate *delegate)
        : m_program(0),
          m_delegate(delegate),
          m_modelViewUniformLocation(0),
          m_projectionUniformLocation(0),
          m_positionAttributeLocation(0),
          m_normalAttributeLocation(0),
          m_message(0)
    {
    }
    ~ShaderProgram() {
        if (m_program) {
            glDeleteProgram(m_program);
            m_program = 0;
        }
        delete[] m_message;
        m_message = 0;
        m_modelViewUniformLocation = 0;
        m_projectionUniformLocation = 0;
        m_positionAttributeLocation = 0;
        m_normalAttributeLocation = 0;
    }

    bool load(const char *vertexShaderSource, const char *fragmentShaderSource) {
        GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
        if (vertexShader && fragmentShader) {
            GLuint program = glCreateProgram();
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            GLint linked;
            glGetProgramiv(program, GL_LINK_STATUS, &linked);
            if (!linked) {
                GLint len;
                glGetShaderiv(program, GL_INFO_LOG_LENGTH, &len);
                if (len > 0) {
                    delete[] m_message;
                    m_message = new char[len];
                    glGetProgramInfoLog(program, len, NULL, m_message);
                    m_delegate->log(IDelegate::kLogWarning, "%s", m_message);
                }
                glDeleteProgram(program);
                return false;
            }
            m_modelViewUniformLocation = glGetUniformLocation(program, "modelViewMatrix");
            m_projectionUniformLocation = glGetUniformLocation(program, "projectionMatrix");
            m_positionAttributeLocation = glGetAttribLocation(program, "inPosition");
            m_normalAttributeLocation = glGetAttribLocation(program, "inNormal");
            m_program = program;
            m_delegate->log(IDelegate::kLogInfo, "Created a shader program (ID=%d)", m_program);
            return true;
        }
        else {
            return false;
        }
    }
    void bind() {
        glUseProgram(m_program);
    }
    void unbind() {
        glUseProgram(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    void setModelViewMatrix(const float value[16]) {
        glUniformMatrix4fv(m_modelViewUniformLocation, 1, GL_FALSE, value);
    }
    void setProjectionMatrix(const float value[16]) {
        glUniformMatrix4fv(m_projectionUniformLocation, 1, GL_FALSE, value);
    }
    void setPosition(const GLvoid *ptr, GLsizei stride) {
        glEnableVertexAttribArray(m_positionAttributeLocation);
        glVertexAttribPointer(m_positionAttributeLocation, 4, GL_FLOAT, GL_FALSE, stride, ptr);
    }
    void setNormal(const GLvoid *ptr, GLsizei stride) {
        glEnableVertexAttribArray(m_normalAttributeLocation);
        glVertexAttribPointer(m_normalAttributeLocation, 3, GL_FLOAT, GL_FALSE, stride, ptr);
    }

protected:
    GLuint m_program;

private:
    GLuint compileShader(const char *source, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint len;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (len > 0) {
                delete[] m_message;
                m_message = new char[len];
                glGetShaderInfoLog(shader, len, NULL, m_message);
                m_delegate->log(IDelegate::kLogWarning, "%s", m_message);
            }
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    IDelegate *m_delegate;
    GLuint m_modelViewUniformLocation;
    GLuint m_projectionUniformLocation;
    GLuint m_positionAttributeLocation;
    GLuint m_normalAttributeLocation;
    char *m_message;
};

class EdgeProgram : public ShaderProgram {
public:
    EdgeProgram(IDelegate *delegate)
        : ShaderProgram(delegate),
          m_colorUniformLocation(0)
    {
    }
    ~EdgeProgram() {
        m_colorUniformLocation = 0;
    }

    bool load(const char *vertexShaderSource, const char *fragmentShaderSource) {
        bool ret = ShaderProgram::load(vertexShaderSource, fragmentShaderSource);
        if (ret)
            m_colorUniformLocation = glGetUniformLocation(m_program, "color");
        return ret;
    }
    void setColor(const btVector3 &value) {
        glUniform4fv(m_colorUniformLocation, 1, value);
    }

private:
    GLuint m_colorUniformLocation;
};

class ShadowProgram : public ShaderProgram {
public:
    ShadowProgram(IDelegate *delegate)
        : ShaderProgram(delegate),
          m_lightColorUniformLocation(0),
          m_lightPositionUniformLocation(0),
          m_lightAmbientUniformLocation(0),
          m_lightDiffuseUniformLocation(0),
          m_lightSpecularUniformLocation(0)
    {
    }
    ~ShadowProgram() {
        m_lightColorUniformLocation = 0;
        m_lightPositionUniformLocation = 0;
        m_lightAmbientUniformLocation = 0;
        m_lightDiffuseUniformLocation = 0;
        m_lightSpecularUniformLocation = 0;
    }

    bool load(const char *vertexShaderSource, const char *fragmentShaderSource) {
        bool ret = ShaderProgram::load(vertexShaderSource, fragmentShaderSource);
        if (ret) {
            m_lightColorUniformLocation = glGetUniformLocation(m_program, "lightColor");
            m_lightPositionUniformLocation = glGetUniformLocation(m_program, "lightPosition");
            m_lightAmbientUniformLocation = glGetUniformLocation(m_program, "lightAmbient");
            m_lightDiffuseUniformLocation = glGetUniformLocation(m_program, "lightDiffuse");
            m_lightSpecularUniformLocation = glGetUniformLocation(m_program, "lightSpecular");
        }
        return ret;
    }
    void setLightColor(const btVector4 &value) {
        glUniform4fv(m_lightColorUniformLocation, 1, value);
    }
    void setLightPosition(const btVector3 &value) {
        glUniform3fv(m_lightPositionUniformLocation, 1, value);
    }
    void setLightAmbient(const btVector4 &value) {
        glUniform4fv(m_lightAmbientUniformLocation, 1, value);
    }
    void setLightDiffuse(const btVector4 &value) {
        glUniform4fv(m_lightDiffuseUniformLocation, 1, value);
    }
    void setLightSpecular(const btVector4 &value) {
        glUniform4fv(m_lightSpecularUniformLocation, 1, value);
    }

private:
    GLuint m_lightColorUniformLocation;
    GLuint m_lightPositionUniformLocation;
    GLuint m_lightAmbientUniformLocation;
    GLuint m_lightDiffuseUniformLocation;
    GLuint m_lightSpecularUniformLocation;
};

class ModelProgram : public ShadowProgram {
public:
    ModelProgram(IDelegate *delegate)
        : ShadowProgram(delegate),
          m_texCoordAttributeLocation(0),
          m_toonTexCoordAttributeLocation(0),
          m_normalMatrixUniformLocation(0),
          m_materialAmbientUniformLocation(0),
          m_materialDiffuseUniformLocation(0),
          m_materialSpecularUniformLocation(0),
          m_materialShininessUniformLocation(0),
          m_lightIntensityUniformLocation(0),
          m_hasMainTextureUniformLocation(0),
          m_hasSubTextureUniformLocation(0),
          m_isMainSphereMapUniformLocation(0),
          m_isSubSphereMapUniformLocation(0),
          m_isMainAdditiveUniformLocation(0),
          m_isSubAdditiveUniformLocation(0),
          m_mainTextureUniformLocation(0),
          m_subTextureUniformLocation(0),
          m_toonTextureUniformLocation(0)
    {
    }
    ~ModelProgram() {
        m_texCoordAttributeLocation = 0;
        m_toonTexCoordAttributeLocation = 0;
        m_normalMatrixUniformLocation = 0;
        m_materialAmbientUniformLocation = 0;
        m_materialDiffuseUniformLocation = 0;
        m_materialSpecularUniformLocation = 0;
        m_materialShininessUniformLocation = 0;
        m_lightIntensityUniformLocation = 0;
        m_hasMainTextureUniformLocation = 0;
        m_hasSubTextureUniformLocation = 0;
        m_isMainSphereMapUniformLocation = 0;
        m_isSubSphereMapUniformLocation = 0;
        m_isMainAdditiveUniformLocation = 0;
        m_isSubAdditiveUniformLocation = 0;
        m_mainTextureUniformLocation = 0;
        m_subTextureUniformLocation = 0;
        m_toonTextureUniformLocation = 0;
    }

    bool load(const char *vertexShaderSource, const char *fragmentShaderSource) {
        bool ret = ShadowProgram::load(vertexShaderSource, fragmentShaderSource);
        if (ret) {
            m_texCoordAttributeLocation = glGetAttribLocation(m_program, "inTexCoord");
            m_toonTexCoordAttributeLocation = glGetAttribLocation(m_program, "inToonTexCoord");
            m_normalMatrixUniformLocation = glGetUniformLocation(m_program, "normalMatrix");
            m_materialAmbientUniformLocation = glGetUniformLocation(m_program, "materialAmbient");
            m_materialDiffuseUniformLocation = glGetUniformLocation(m_program, "materialDiffuse");
            m_materialSpecularUniformLocation = glGetUniformLocation(m_program, "materialSpecular");
            m_materialShininessUniformLocation = glGetUniformLocation(m_program, "materialShininess");
            m_lightIntensityUniformLocation = glGetUniformLocation(m_program, "lightIntensity");
            m_hasMainTextureUniformLocation = glGetUniformLocation(m_program, "hasMainTexture");
            m_hasSubTextureUniformLocation = glGetUniformLocation(m_program, "hasSubTexture");
            m_isMainSphereMapUniformLocation = glGetUniformLocation(m_program, "isMainSphereMap");
            m_isSubSphereMapUniformLocation = glGetUniformLocation(m_program, "isSubSphereMap");
            m_isMainAdditiveUniformLocation = glGetUniformLocation(m_program, "isMainAdditive");
            m_isSubAdditiveUniformLocation = glGetUniformLocation(m_program, "isSubAdditive");
            m_mainTextureUniformLocation = glGetUniformLocation(m_program, "mainTexture");
            m_subTextureUniformLocation = glGetUniformLocation(m_program, "subTexture");
            m_toonTextureUniformLocation = glGetUniformLocation(m_program, "toonTexture");
        }
        return ret;
    }
    void bind() {
        ShadowProgram::bind();
        resetTextureState();
    }
    void resetTextureState() {
        glUniform1i(m_hasMainTextureUniformLocation, 0);
        glUniform1i(m_hasSubTextureUniformLocation, 0);
        glUniform1i(m_isMainSphereMapUniformLocation, 0);
        glUniform1i(m_isSubSphereMapUniformLocation, 0);
        glUniform1i(m_isMainAdditiveUniformLocation, 0);
        glUniform1i(m_isSubAdditiveUniformLocation, 0);
    }
    void setTexCoord(const GLvoid *ptr, GLsizei stride) {
        glEnableVertexAttribArray(m_texCoordAttributeLocation);
        glVertexAttribPointer(m_texCoordAttributeLocation, 2, GL_FLOAT, GL_FALSE, stride, ptr);
    }
    void setToonTexCoord(const GLvoid *ptr, GLsizei stride) {
        glEnableVertexAttribArray(m_toonTexCoordAttributeLocation);
        glVertexAttribPointer(m_toonTexCoordAttributeLocation, 2, GL_FLOAT, GL_FALSE, stride, ptr);
    }
    void setNormalMatrix(const float value[9]) {
        glUniformMatrix3fv(m_normalMatrixUniformLocation, 1, GL_FALSE, value);
    }
    void setMaterialAmbient(const btVector4 &value) {
        glUniform4fv(m_materialAmbientUniformLocation, 1, value);
    }
    void setMaterialDiffuse(const btVector4 &value) {
        glUniform4fv(m_materialDiffuseUniformLocation, 1, value);
    }
    void setMaterialSpecular(const btVector4 &value) {
        glUniform4fv(m_materialSpecularUniformLocation, 1, value);
    }
    void setMaterialShininess(float value) {
        glUniform1f(m_materialShininessUniformLocation, value);
    }
    void setLightIntensity(float value) {
        glUniform1f(m_lightIntensityUniformLocation, value);
    }
    void setIsMainSphereMap(bool value) {
        glUniform1i(m_isMainSphereMapUniformLocation, value ? 1 : 0);
    }
    void setIsSubSphereMap(bool value) {
        glUniform1i(m_isSubSphereMapUniformLocation, value ? 1 : 0);
    }
    void setIsMainAdditive(bool value) {
        glUniform1i(m_isMainAdditiveUniformLocation, value ? 1 : 0);
    }
    void setIsSubAdditive(bool value) {
        glUniform1i(m_isSubAdditiveUniformLocation, value ? 1 : 0);
    }
    void setMainTexture(GLuint value) {
        if (value) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, value);
            glUniform1i(m_mainTextureUniformLocation, 0);
            glUniform1i(m_hasMainTextureUniformLocation, 1);
        }
    }
    void setSubTexture(GLuint value) {
        if (value) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, value);
            glUniform1i(m_subTextureUniformLocation, 2);
            glUniform1i(m_hasSubTextureUniformLocation, 1);
        }
    }
    void setToonTexture(GLuint value) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, value);
        glUniform1i(m_toonTextureUniformLocation, 1);
    }

private:
    GLuint m_texCoordAttributeLocation;
    GLuint m_toonTexCoordAttributeLocation;
    GLuint m_normalMatrixUniformLocation;
    GLuint m_materialAmbientUniformLocation;
    GLuint m_materialDiffuseUniformLocation;
    GLuint m_materialSpecularUniformLocation;
    GLuint m_materialShininessUniformLocation;
    GLuint m_lightIntensityUniformLocation;
    GLuint m_hasMainTextureUniformLocation;
    GLuint m_hasSubTextureUniformLocation;
    GLuint m_isMainSphereMapUniformLocation;
    GLuint m_isSubSphereMapUniformLocation;
    GLuint m_isMainAdditiveUniformLocation;
    GLuint m_isSubAdditiveUniformLocation;
    GLuint m_mainTextureUniformLocation;
    GLuint m_subTextureUniformLocation;
    GLuint m_toonTextureUniformLocation;
};

class AssetProgram : public ShadowProgram {
public:
    AssetProgram(IDelegate *delegate)
        : ShadowProgram(delegate),
          m_texCoordAttributeLocation(0),
          m_colorAttributeLocation(0),
          m_normalMatrixUniformLocation(0),
          m_transformMatrixUniformLocation(0),
          m_materialAmbientUniformLocation(0),
          m_materialDiffuseUniformLocation(0),
          m_materialSpecularUniformLocation(0),
          m_materialShininessUniformLocation(0),
          m_hasTextureUniformLocation(0),
          m_textureUniformLocation(0),
          m_opacityUniformLocation(0)
    {
    }
    ~AssetProgram() {
        m_texCoordAttributeLocation = 0;
        m_colorAttributeLocation = 0;
        m_normalMatrixUniformLocation = 0;
        m_transformMatrixUniformLocation = 0;
        m_materialAmbientUniformLocation = 0;
        m_materialDiffuseUniformLocation = 0;
        m_materialEmissionUniformLocation = 0;
        m_materialSpecularUniformLocation = 0;
        m_materialShininessUniformLocation = 0;
        m_hasTextureUniformLocation = 0;
        m_textureUniformLocation = 0;
        m_opacityUniformLocation = 0;
    }

    bool load(const char *vertexShaderSource, const char *fragmentShaderSource) {
        bool ret = ShadowProgram::load(vertexShaderSource, fragmentShaderSource);
        if (ret) {
            m_texCoordAttributeLocation = glGetAttribLocation(m_program, "inTexCoord");
            m_colorAttributeLocation = glGetAttribLocation(m_program, "inColor");
            m_normalMatrixUniformLocation = glGetUniformLocation(m_program, "normalMatrix");
            m_transformMatrixUniformLocation = glGetUniformLocation(m_program, "transformMatrix");
            m_materialAmbientUniformLocation = glGetUniformLocation(m_program, "materialAmbient");
            m_materialDiffuseUniformLocation = glGetUniformLocation(m_program, "materialDiffuse");
            m_materialEmissionUniformLocation = glGetUniformLocation(m_program, "materialEmission");
            m_materialSpecularUniformLocation = glGetUniformLocation(m_program, "materialSpecular");
            m_materialShininessUniformLocation = glGetUniformLocation(m_program, "materialShininess");
            m_hasTextureUniformLocation = glGetUniformLocation(m_program, "hasTexture");
            m_textureUniformLocation = glGetUniformLocation(m_program, "mainTexture");
            m_opacityUniformLocation = glGetUniformLocation(m_program, "opacity");
        }
        return ret;
    }
    void bind() {
        ShadowProgram::bind();
        resetTextureState();
    }
    void resetTextureState() {
        glUniform1i(m_hasTextureUniformLocation, 0);
    }
    void setTexCoord(const GLvoid *ptr, GLsizei stride) {
        glEnableVertexAttribArray(m_texCoordAttributeLocation);
        glVertexAttribPointer(m_texCoordAttributeLocation, 2, GL_FLOAT, GL_FALSE, stride, ptr);
    }
    void setColor(const GLvoid *ptr, GLsizei stride) {
        glEnableVertexAttribArray(m_colorAttributeLocation);
        glVertexAttribPointer(m_colorAttributeLocation, 4, GL_FLOAT, GL_FALSE, stride, ptr);
    }
    void setNormalMatrix(const float value[16]) {
        glUniformMatrix3fv(m_normalMatrixUniformLocation, 1, GL_FALSE, value);
    }
    void setTransformMatrix(const float value[9]) {
        glUniformMatrix4fv(m_transformMatrixUniformLocation, 1, GL_FALSE, value);
    }
    void setMaterialAmbient(const btVector4 &value) {
        glUniform4fv(m_materialAmbientUniformLocation, 1, value);
    }
    void setMaterialDiffuse(const btVector4 &value) {
        glUniform4fv(m_materialDiffuseUniformLocation, 1, value);
    }
    void setMaterialEmission(const btVector4 &value) {
        glUniform4fv(m_materialEmissionUniformLocation, 1, value);
    }
    void setMaterialSpecular(const btVector4 &value) {
        glUniform4fv(m_materialSpecularUniformLocation, 1, value);
    }
    void setMaterialShininess(float value) {
        glUniform1f(m_materialShininessUniformLocation, value);
    }
    void setOpacity(float value) {
        glUniform1f(m_opacityUniformLocation, value);
    }
    void setTexture(GLuint value) {
        if (value) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, value);
            glUniform1i(m_textureUniformLocation, 0);
            glUniform1i(m_hasTextureUniformLocation, 1);
        }
    }

private:
    GLuint m_texCoordAttributeLocation;
    GLuint m_colorAttributeLocation;
    GLuint m_normalMatrixUniformLocation;
    GLuint m_transformMatrixUniformLocation;
    GLuint m_materialAmbientUniformLocation;
    GLuint m_materialDiffuseUniformLocation;
    GLuint m_materialEmissionUniformLocation;
    GLuint m_materialSpecularUniformLocation;
    GLuint m_materialShininessUniformLocation;
    GLuint m_hasTextureUniformLocation;
    GLuint m_textureUniformLocation;
    GLuint m_opacityUniformLocation;
};

}
}

#ifdef VPVL_LINK_ASSIMP
namespace
{
struct AssetVertex
{
    btVector3 position;
    btVector3 normal;
    btVector3 texcoord;
    btVector4 color;
};
struct AssetVBO
{
    GLuint vertices;
    GLuint indices;
};
typedef btAlignedObjectArray<AssetVertex> AssetVertices;
typedef btAlignedObjectArray<uint32_t> AssetIndices;
}
namespace vpvl
{
struct AssetUserData
{
    std::map<std::string, GLuint> textures;
    std::map<const struct aiMesh *, AssetVertices> vertices;
    std::map<const struct aiMesh *, AssetIndices> indices;
    std::map<const struct aiMesh *, AssetVBO> vbo;
    std::map<const struct aiNode *, vpvl::gl2::AssetProgram *> programs;
};
}
namespace
{

void aiLoadAssetRecursive(const aiScene *scene, const aiNode *node, vpvl::AssetUserData *userData, vpvl::gl2::IDelegate *delegate)
{
    const uint32_t nMeshes = node->mNumMeshes;
    AssetVertex assetVertex;
    vpvl::gl2::AssetProgram *program = new vpvl::gl2::AssetProgram(delegate);
    program->load(delegate->loadShader(vpvl::gl2::IDelegate::kAssetVertexShader).c_str(),
                  delegate->loadShader(vpvl::gl2::IDelegate::kAssetFragmentShader).c_str());
    userData->programs[node] = program;
    for (uint32_t i = 0; i < nMeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const aiVector3D *vertices = mesh->mVertices;
        const aiVector3D *normals = mesh->mNormals;
        const bool hasNormals = mesh->HasNormals();
        const bool hasColors = mesh->HasVertexColors(0);
        const bool hasTexCoords = mesh->HasTextureCoords(0);
        const aiColor4D *colors = hasColors ? mesh->mColors[0] : 0;
        const aiVector3D *texcoords = hasTexCoords ? mesh->mTextureCoords[0] : 0;
        AssetVertices &assetVertices = userData->vertices[mesh];
        AssetIndices &indices = userData->indices[mesh];
        const uint32_t nFaces = mesh->mNumFaces;
        uint32_t index = 0;
        for (uint32_t j = 0; j < nFaces; j++) {
            const struct aiFace &face = mesh->mFaces[j];
            const uint32_t nIndices = face.mNumIndices;
            for (uint32_t k = 0; k < nIndices; k++) {
                int vertexIndex = face.mIndices[k];
                if (hasColors) {
                    const aiColor4D &c = colors[vertexIndex];
                    assetVertex.color.setValue(c.r, c.g, c.b, c.a);
                }
                else {
                    assetVertex.color.setZero();
                    assetVertex.color.setW(1.0f);
                }
                if (hasTexCoords) {
                    const aiVector3D &p = texcoords[vertexIndex];
                    assetVertex.texcoord.setValue(p.x, p.y, 0.0f);
                }
                else {
                    assetVertex.texcoord.setZero();
                }
                if (hasNormals) {
                    const aiVector3D &n = normals[vertexIndex];
                    assetVertex.normal.setValue(n.x, n.y, n.z);
                }
                else {
                    assetVertex.normal.setZero();
                }
                const aiVector3D &v = vertices[vertexIndex];
                assetVertex.position.setValue(v.x, v.y, v.z);
                assetVertex.position.setW(1.0f);
                assetVertices.push_back(assetVertex);
                indices.push_back(index);
                index++;
            }
        }
        AssetVBO &vbo = userData->vbo[mesh];
        size_t vsize = assetVertices.size() * sizeof(assetVertices[0]);
        glGenBuffers(1, &vbo.vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.vertices);
        glBufferData(GL_ARRAY_BUFFER, vsize, assetVertices[0].position, GL_STATIC_DRAW);
        glGenBuffers(1, &vbo.indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);
    }
    const uint32_t nChildNodes = node->mNumChildren;
    for (uint32_t i = 0; i < nChildNodes; i++)
        aiLoadAssetRecursive(scene, node->mChildren[i], userData, delegate);
}

void aiUnloadAssetRecursive(const aiScene *scene, const aiNode *node, vpvl::AssetUserData *userData)
{
    const uint32_t nMeshes = node->mNumMeshes;
    for (uint32_t i = 0; i < nMeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const AssetVBO &vbo = userData->vbo[mesh];
        glDeleteBuffers(1, &vbo.vertices);
        glDeleteBuffers(1, &vbo.indices);
    }
    delete userData->programs[node];
    const uint32_t nChildNodes = node->mNumChildren;
    for (uint32_t i = 0; i < nChildNodes; i++)
        aiUnloadAssetRecursive(scene, node->mChildren[i], userData);
}

void aiSetAssetMaterial(const aiMaterial *material, vpvl::Asset *asset, vpvl::gl2::AssetProgram *program)
{
    int textureIndex = 0;
    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath) == aiReturn_SUCCESS) {
        GLuint textureID = asset->userData()->textures[texturePath.data];
        program->setTexture(textureID);
    }
    else {
        program->setTexture(0);
    }
    aiColor4D ambient, diffuse, emission, specular;
    btVector4 color(0.0f, 0.0f, 0.0f, 0.0f);
    if (aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient) == aiReturn_SUCCESS) {
        color.setValue(ambient.r, ambient.g, ambient.b, ambient.a);
        program->setMaterialAmbient(color);
    }
    else {
        program->setMaterialAmbient(btVector4(0.2f, 0.2f, 0.2f, 1.0f));
    }
    if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == aiReturn_SUCCESS) {
        color.setValue(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
        program->setMaterialDiffuse(color);
    }
    else {
        program->setMaterialDiffuse(btVector4(0.8f, 0.8f, 0.8f, 1.0f));
    }
    if (aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emission) == aiReturn_SUCCESS) {
        color.setValue(emission.r, emission.g, emission.b, emission.a);
        program->setMaterialEmission(color);
    }
    else {
        program->setMaterialEmission(btVector4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular) == aiReturn_SUCCESS) {
        color.setValue(specular.r, specular.g, specular.b, specular.a);
        program->setMaterialSpecular(color);
    }
    else {
        program->setMaterialSpecular(btVector4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    float shininess, strength;
    int ret1 = aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess);
    int ret2 = aiGetMaterialFloat(material, AI_MATKEY_SHININESS_STRENGTH, &strength);
    if (ret1 == aiReturn_SUCCESS && ret2 == aiReturn_SUCCESS) {
        program->setMaterialShininess(shininess * strength);
    }
    else if (ret1 == aiReturn_SUCCESS) {
        program->setMaterialShininess(shininess);
    }
    else {
        program->setMaterialShininess(15.0f);
    }
    float opacity;
    if (aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity) == aiReturn_SUCCESS) {
        program->setOpacity(opacity * asset->opacity());
    }
    else {
        program->setOpacity(asset->opacity());
    }
    int wireframe, twoside;
    if (aiGetMaterialInteger(material, AI_MATKEY_ENABLE_WIREFRAME, &wireframe) == aiReturn_SUCCESS && wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (aiGetMaterialInteger(material, AI_MATKEY_TWOSIDED, &twoside) == aiReturn_SUCCESS && twoside)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void aiDrawAssetRecurse(const aiScene *scene, const aiNode *node, vpvl::Asset *asset, vpvl::Scene *s)
{
    const btScalar &scaleFactor = asset->scaleFactor();
    const vpvl::Bone *bone = asset->parentBone();
    float matrix4x4[16], matrix3x3[9];
    aiVector3D aiS, aiP;
    aiQuaternion aiQ;
    node->mTransformation.Decompose(aiS, aiQ, aiP);
    btTransform transform(btMatrix3x3(btQuaternion(aiQ.x, aiQ.y, aiQ.z, aiQ.w) * asset->rotation())
                          .scaled(btVector3(aiS.x * scaleFactor, aiS.y * scaleFactor, aiS.z * scaleFactor)),
                          btVector3(aiP.x,aiP.y, aiP.z) + asset->position());
    if (bone) {
        const btTransform &boneTransform = bone->localTransform();
        transform.setBasis(boneTransform.getBasis() * transform.getBasis());
        transform.setOrigin(boneTransform.getOrigin() + transform.getOrigin());
    }
    vpvl::AssetUserData *userData = asset->userData();
    AssetVertex v;
    const GLvoid *vertexPtr = 0;
    const GLvoid *normalPtr = reinterpret_cast<const GLvoid *>(reinterpret_cast<const uint8_t *>(&v.normal) - reinterpret_cast<const uint8_t *>(&v.position));
    const GLvoid *texcoordPtr = reinterpret_cast<const GLvoid *>(reinterpret_cast<const uint8_t *>(&v.texcoord) - reinterpret_cast<const uint8_t *>(&v.position));
    const GLvoid *colorPtr = reinterpret_cast<const GLvoid *>(reinterpret_cast<const uint8_t *>(&v.color) - reinterpret_cast<const uint8_t *>(&v.position));
    const uint32_t nMeshes = node->mNumMeshes;
    const size_t stride = sizeof(AssetVertex);
    vpvl::gl2::AssetProgram *program = userData->programs[node];
    program->bind();
    s->getModelViewMatrix(matrix4x4);
    program->setModelViewMatrix(matrix4x4);
    s->getProjectionMatrix(matrix4x4);
    program->setProjectionMatrix(matrix4x4);
    s->getNormalMatrix(matrix3x3);
    program->setNormalMatrix(matrix3x3);
    transform.getOpenGLMatrix(matrix4x4);
    program->setTransformMatrix(matrix4x4);
    program->setLightAmbient(s->lightAmbient());
    program->setLightColor(s->lightColor());
    program->setLightDiffuse(s->lightDiffuse());
    program->setLightPosition(s->lightPosition());
    program->setLightSpecular(s->lightSpecular());
    for (uint32_t i = 0; i < nMeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const AssetVBO &vbo = userData->vbo[mesh];
        const AssetIndices &indices = userData->indices[mesh];
        aiSetAssetMaterial(scene->mMaterials[mesh->mMaterialIndex], asset, program);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.vertices);
        program->setPosition(vertexPtr, stride);
        program->setNormal(normalPtr, stride);
        program->setTexCoord(texcoordPtr, stride);
        program->setColor(colorPtr, stride);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.indices);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        program->resetTextureState();
    }
    program->unbind();
    const uint32_t nChildNodes = node->mNumChildren;
    for (uint32_t i = 0; i < nChildNodes; i++)
        aiDrawAssetRecurse(scene, node->mChildren[i], asset, s);
}

void aiDrawAsset(vpvl::Asset *asset, vpvl::Scene *s)
{
    const aiScene *a = asset->getScene();
    aiDrawAssetRecurse(a, a->mRootNode, asset, s);
}

}
#else
VPVL_DECLARE_HANDLE(aiNode)
VPVL_DECLARE_HANDLE(aiScene)
namespace vpvl
{
    namespace gl
    {
    VPVL_DECLARE_HANDLE(AssetInternal)
    }
}
namespace
{
void aiDrawAsset(vpvl::Asset *asset, vpvl::Scene *s)
{
    (void) asset;
    (void) s;
}
}
#endif

namespace {

enum VertexBufferObjectType {
    kModelVertices,
    kModelNormals,
    kModelColors,
    kModelTexCoords,
    kModelToonTexCoords,
    kEdgeVertices,
    kEdgeIndices,
    kShadowIndices,
    kVertexBufferObjectMax
};

struct PMDModelMaterialPrivate {
    GLuint mainTextureID;
    GLuint subTextureID;
};

const std::string CanonicalizePath(const std::string &path)
{
    const std::string from("\\"), to("/");
    std::string ret(path);
    std::string::size_type pos(path.find(from));
    while (pos != std::string::npos) {
        ret.replace(pos, from.length(), to);
        pos = ret.find(from, pos + to.length());
    }
    return ret;
}

}

namespace vpvl
{

struct PMDModelUserData {
    GLuint toonTextureID[vpvl::PMDModel::kSystemTextureMax];
    GLuint vertexBufferObjects[kVertexBufferObjectMax];
    bool hasSingleSphereMap;
    bool hasMultipleSphereMap;
    PMDModelMaterialPrivate *materials;
};

namespace gl2
{

class DebugDrawer : public btIDebugDraw
{
public:
    DebugDrawer() : m_world(0) {}
    virtual ~DebugDrawer() {}

    void draw3dText(const btVector3 & /* location */, const char *textString) {
        fprintf(stderr, "[INFO]: %s\n", textString);
    }
    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int /* lifeTime */, const btVector3 &color) {
        const btVector3 to = PointOnB + normalOnB * distance;
        glBegin(GL_LINES);
        glColor3fv(color);
        glVertex3fv(PointOnB);
        glVertex3fv(to);
        glEnd();
    }
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
        glBegin(GL_LINES);
        glColor3fv(color);
        glVertex3fv(from);
        glVertex3fv(to);
        glEnd();
    }
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor) {
        glBegin(GL_LINES);
        glColor3fv(fromColor);
        glVertex3fv(from);
        glColor3fv(toColor);
        glVertex3fv(to);
        glEnd();
    }
    void reportErrorWarning(const char *warningString) {
        fprintf(stderr, "[ERROR]: %s\n", warningString);
    }
    int getDebugMode() const {
        return m_flags;
    }
    void setDebugMode(int debugMode) {
        m_flags = debugMode;
    }

    void render() {
        if (m_world)
            m_world->debugDrawWorld();
    }
    void setWorld(btDynamicsWorld *value) {
        m_world = value;
    }

private:
    btDynamicsWorld *m_world;
    int m_flags;
};

bool Renderer::initializeGLEW(GLenum &err)
{
#ifndef VPVL_USE_ALLEGRO5
    err = glewInit();
    return err == GLEW_OK;
#else
    (void) err;
    return true;
#endif
}

Renderer::Renderer(IDelegate *delegate, int width, int height, int fps)
    : m_delegate(delegate),
      m_edgeProgram(0),
      m_modelProgram(0),
      m_shadowProgram(0),
      m_scene(0),
      m_selected(0),
      m_debugDrawer(0)
{
    m_edgeProgram = new EdgeProgram(delegate);
    m_modelProgram = new ModelProgram(delegate);
    m_shadowProgram = new ShadowProgram(delegate);
    m_debugDrawer = new DebugDrawer();
    m_scene = new vpvl::Scene(width, height, fps);
}

Renderer::~Renderer()
{
    vpvl::Array<vpvl::PMDModel *> models;
    models.copy(m_scene->models());
    const uint32_t nModels = models.count();
    for (uint32_t i = 0; i < nModels; i++) {
        vpvl::PMDModel *model = models[i];
        unloadModel(model);
    }
    vpvl::Array<vpvl::Asset *> assets;
    assets.copy(m_assets);
    const uint32_t nAssets = assets.count();
    for (uint32_t i = 0; i < nAssets; i++) {
        vpvl::Asset *asset = assets[i];
        unloadAsset(asset);
    }
    models.releaseAll();
    assets.clear();
    delete m_edgeProgram;
    m_edgeProgram = 0;
    delete m_modelProgram;
    m_modelProgram = 0;
    delete m_shadowProgram;
    m_shadowProgram = 0;
    delete m_debugDrawer;
    m_debugDrawer = 0;
    delete m_scene;
    m_scene = 0;
}

void Renderer::initializeSurface()
{
    glClearStencil(0);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool Renderer::createPrograms()
{
    bool ret = false;
    std::string vertexShader;
    std::string fragmentShader;
    vertexShader = m_delegate->loadShader(IDelegate::kEdgeVertexShader);
    fragmentShader = m_delegate->loadShader(IDelegate::kEdgeFragmentShader);
    ret = m_edgeProgram->load(vertexShader.c_str(), fragmentShader.c_str());
    if (!ret)
        return ret;
    vertexShader = m_delegate->loadShader(IDelegate::kModelVertexShader);
    fragmentShader = m_delegate->loadShader(IDelegate::kModelFragmentShader);
    ret = m_modelProgram->load(vertexShader.c_str(), fragmentShader.c_str());
    if (!ret)
        return ret;
    vertexShader = m_delegate->loadShader(IDelegate::kShadowVertexShader);
    fragmentShader = m_delegate->loadShader(IDelegate::kShadowFragmentShader);
    ret = m_shadowProgram->load(vertexShader.c_str(), fragmentShader.c_str());
    return ret;
}

void Renderer::resize(int width, int height)
{
    m_scene->setWidth(width);
    m_scene->setHeight(height);
}

void Renderer::setDebugDrawer(btDynamicsWorld *world)
{
    static_cast<DebugDrawer *>(m_debugDrawer)->setWorld(world);
    world->setDebugDrawer(m_debugDrawer);
}

void Renderer::loadModel(vpvl::PMDModel *model, const std::string &dir)
{
    const vpvl::MaterialList &materials = model->materials();
    const uint32_t nMaterials = materials.count();
    GLuint textureID = 0;
    vpvl::PMDModelUserData *userData = new vpvl::PMDModelUserData;
    PMDModelMaterialPrivate *materialPrivates = new PMDModelMaterialPrivate[nMaterials];
    bool hasSingleSphere = false, hasMultipleSphere = false;
    for (uint32_t i = 0; i < nMaterials; i++) {
        const vpvl::Material *material = materials[i];
        const std::string primary = m_delegate->toUnicode(material->mainTextureName());
        const std::string second = m_delegate->toUnicode(material->subTextureName());
        PMDModelMaterialPrivate &materialPrivate = materialPrivates[i];
        materialPrivate.mainTextureID = 0;
        materialPrivate.subTextureID = 0;
        if (!primary.empty()) {
            if (m_delegate->loadTexture(dir + "/" + primary, textureID)) {
                materialPrivate.mainTextureID = textureID;
                m_delegate->log(IDelegate::kLogInfo, "Binding the texture as a primary texture (ID=%d)", textureID);
            }
        }
        if (!second.empty()) {
            if (m_delegate->loadTexture(dir + "/" + second, textureID)) {
                materialPrivate.subTextureID = textureID;
                m_delegate->log(IDelegate::kLogInfo, "Binding the texture as a secondary texture (ID=%d)", textureID);
            }
        }
        hasSingleSphere |= material->isMainSphereModulate() && !material->isSubSphereAdd();
        hasMultipleSphere |= material->isSubSphereAdd();
    }
    userData->hasSingleSphereMap = hasSingleSphere;
    userData->hasMultipleSphereMap = hasMultipleSphere;
    m_delegate->log(IDelegate::kLogInfo,
                    "Sphere map information: hasSingleSphere=%s, hasMultipleSphere=%s",
                    hasSingleSphere ? "true" : "false",
                    hasMultipleSphere ? "true" : "false");
    glGenBuffers(kVertexBufferObjectMax, userData->vertexBufferObjects);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, userData->vertexBufferObjects[kEdgeIndices]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->edgeIndicesCount() * model->strideSize(vpvl::PMDModel::kEdgeIndicesStride),
                 model->edgeIndicesPointer(), GL_STATIC_DRAW);
    m_delegate->log(IDelegate::kLogInfo,
                    "Binding edge indices to the vertex buffer object (ID=%d)",
                    userData->vertexBufferObjects[kEdgeIndices]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, userData->vertexBufferObjects[kShadowIndices]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->indices().count() * model->strideSize(vpvl::PMDModel::kIndicesStride),
                 model->indicesPointer(), GL_STATIC_DRAW);
    m_delegate->log(IDelegate::kLogInfo,
                    "Binding indices to the vertex buffer object (ID=%d)",
                    userData->vertexBufferObjects[kShadowIndices]);
    glBindBuffer(GL_ARRAY_BUFFER, userData->vertexBufferObjects[kModelVertices]);
    glBufferData(GL_ARRAY_BUFFER, model->vertices().count() * model->strideSize(vpvl::PMDModel::kVerticesStride),
                 model->verticesPointer(), GL_DYNAMIC_DRAW);
    m_delegate->log(IDelegate::kLogInfo,
                    "Binding texture coordinates to the vertex buffer object (ID=%d)",
                    userData->vertexBufferObjects[kModelTexCoords]);
    if (m_delegate->loadToonTexture("toon0.bmp", dir, textureID)) {
        userData->toonTextureID[0] = textureID;
        glTexParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_delegate->log(IDelegate::kLogInfo, "Binding the texture as a toon texture (ID=%d)", textureID);
    }
    for (uint32_t i = 0; i < vpvl::PMDModel::kSystemTextureMax - 1; i++) {
        const uint8_t *name = model->toonTexture(i);
        if (m_delegate->loadToonTexture(reinterpret_cast<const char *>(name), dir, textureID)) {
            userData->toonTextureID[i + 1] = textureID;
            glTexParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_delegate->log(IDelegate::kLogInfo, "Binding the texture as a toon texture (ID=%d)", textureID);
        }
    }
    userData->materials = materialPrivates;
    model->setUserData(userData);
    m_delegate->log(IDelegate::kLogInfo, "Created the model: %s", m_delegate->toUnicode(model->name()).c_str());
    m_scene->addModel(model);
}

void Renderer::unloadModel(const vpvl::PMDModel *model)
{
    if (model) {
        const vpvl::MaterialList &materials = model->materials();
        const uint32_t nMaterials = materials.count();
        vpvl::PMDModelUserData *userData = model->userData();
        for (uint32_t i = 0; i < nMaterials; i++) {
            PMDModelMaterialPrivate &materialPrivate = userData->materials[i];
            glDeleteTextures(1, &materialPrivate.mainTextureID);
            glDeleteTextures(1, &materialPrivate.subTextureID);
        }
        for (uint32_t i = 0; i < vpvl::PMDModel::kSystemTextureMax; i++) {
            glDeleteTextures(1, &userData->toonTextureID[i]);
        }
        glDeleteBuffers(kVertexBufferObjectMax, userData->vertexBufferObjects);
        delete[] userData->materials;
        delete userData;
        m_delegate->log(IDelegate::kLogInfo, "Destroyed the model: %s", m_delegate->toUnicode(model->name()).c_str());
        m_scene->removeModel(const_cast<vpvl::PMDModel *>(model));
    }
}

void Renderer::updateModelBuffer(const vpvl::PMDModel *model) const
{
    size_t size = model->vertices().count() * model->strideSize(vpvl::PMDModel::kVerticesStride);
    glBindBuffer(GL_ARRAY_BUFFER, model->userData()->vertexBufferObjects[kModelVertices]);
    glBufferData(GL_ARRAY_BUFFER, size, model->verticesPointer(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::drawModel(const vpvl::PMDModel *model)
{
#ifndef VPVL_COORDINATE_OPENGL
    glPushMatrix();
    glScalef(1.0f, 1.0f, -1.0f);
    glCullFace(GL_FRONT);
#endif

    const vpvl::PMDModelUserData *userData = model->userData();
    size_t stride = model->strideSize(vpvl::PMDModel::kVerticesStride), vsize = model->vertices().count();
    m_modelProgram->bind();
    glBindBuffer(GL_ARRAY_BUFFER, userData->vertexBufferObjects[kModelVertices]);
    m_modelProgram->setPosition(reinterpret_cast<const GLvoid *>(model->strideOffset(vpvl::PMDModel::kVerticesStride)), stride);
    m_modelProgram->setNormal(reinterpret_cast<const GLvoid *>(model->strideOffset(vpvl::PMDModel::kNormalsStride)), stride);
    m_modelProgram->setTexCoord(reinterpret_cast<const GLvoid *>(model->strideOffset(vpvl::PMDModel::kTextureCoordsStride)), stride);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, userData->vertexBufferObjects[kShadowIndices]);

    float matrix4x4[16], matrix3x3[9];
    m_scene->getModelViewMatrix(matrix4x4);
    m_modelProgram->setModelViewMatrix(matrix4x4);
    m_scene->getProjectionMatrix(matrix4x4);
    m_modelProgram->setProjectionMatrix(matrix4x4);
    m_scene->getNormalMatrix(matrix3x3);
    m_modelProgram->setNormalMatrix(matrix3x3);
    m_modelProgram->setLightAmbient(m_scene->lightAmbient());
    m_modelProgram->setLightColor(m_scene->lightColor());
    m_modelProgram->setLightDiffuse(m_scene->lightDiffuse());
    m_modelProgram->setLightPosition(m_scene->lightPosition());
    m_modelProgram->setLightSpecular(m_scene->lightSpecular());
    m_modelProgram->setLightIntensity(m_scene->lightIntensity());

    const bool enableToon = true;
    // toon
    if (enableToon) {
        // shadow map
        stride = model->strideSize(vpvl::PMDModel::kToonTextureStride);
        glBindBuffer(GL_ARRAY_BUFFER, userData->vertexBufferObjects[kModelToonTexCoords]);
        glBufferData(GL_ARRAY_BUFFER, vsize * stride, model->toonTextureCoordsPointer(), GL_DYNAMIC_DRAW);
        m_modelProgram->setToonTexCoord(0, stride);
    }

    const vpvl::MaterialList &materials = model->materials();
    const PMDModelMaterialPrivate *materialPrivates = userData->materials;
    const uint32_t nMaterials = materials.count();
    btVector4 average, ambient, diffuse, specular;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < nMaterials; i++) {
        const vpvl::Material *material = materials[i];
        const PMDModelMaterialPrivate &materialPrivate = materialPrivates[i];
        // toon
        const float alpha = material->opacity();
        if (enableToon) {
            average = material->averageColor();
            average.setW(average.w() * alpha);
            specular = material->specular();
            specular.setW(specular.w() * alpha);
            m_modelProgram->setMaterialAmbient(average);
            m_modelProgram->setMaterialDiffuse(average);
            m_modelProgram->setMaterialSpecular(specular);
        }
        else {
            ambient = material->ambient();
            ambient.setW(ambient.w() * alpha);
            diffuse = material->diffuse();
            diffuse.setW(diffuse.w() * alpha);
            specular = material->specular();
            specular.setW(specular.w() * alpha);
            m_modelProgram->setMaterialAmbient(ambient);
            m_modelProgram->setMaterialDiffuse(diffuse);
            m_modelProgram->setMaterialSpecular(specular);
        }
        m_modelProgram->setMaterialShininess(material->shiness());
        m_modelProgram->setMainTexture(materialPrivate.mainTextureID);
        m_modelProgram->setToonTexture(userData->toonTextureID[material->toonID()]);
        m_modelProgram->setSubTexture(materialPrivate.subTextureID);
        m_modelProgram->setIsMainSphereMap(material->isMainSphereAdd() || material->isMainSphereModulate());
        m_modelProgram->setIsMainAdditive(material->isMainSphereAdd());
        m_modelProgram->setIsSubSphereMap(material->isSubSphereAdd() || material->isSubSphereModulate());
        m_modelProgram->setIsSubAdditive(material->isSubSphereAdd());
        material->opacity() < 1.0f ? glDisable(GL_CULL_FACE) : glEnable(GL_CULL_FACE);

        // draw
        const uint32_t nIndices = material->countIndices();
        glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid *>(offset));
        offset += (nIndices << 1);
        m_modelProgram->resetTextureState();
    }

    m_modelProgram->unbind();
    glEnable(GL_CULL_FACE);

#ifndef VPVL_COORDINATE_OPENGL
    glPopMatrix();
    glCullFace(GL_BACK);
#endif
}

void Renderer::drawModelEdge(const vpvl::PMDModel *model)
{
#ifdef VPVL_COORDINATE_OPENGL
    glCullFace(GL_FRONT);
#else
    glPushMatrix();
    glScalef(1.0f, 1.0f, -1.0f);
    glCullFace(GL_BACK);
#endif

    const size_t stride = model->strideSize(vpvl::PMDModel::kEdgeVerticesStride);
    const vpvl::PMDModelUserData *modelPrivate = model->userData();

    float modelViewMatrix[16], projectionMatrix[16];
    m_scene->getModelViewMatrix(modelViewMatrix);
    m_scene->getProjectionMatrix(projectionMatrix);

    m_edgeProgram->bind();
    size_t len = model->vertices().count() * stride;
    glBindBuffer(GL_ARRAY_BUFFER, modelPrivate->vertexBufferObjects[kEdgeVertices]);
    glBufferData(GL_ARRAY_BUFFER, len, model->edgeVerticesPointer(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelPrivate->vertexBufferObjects[kEdgeIndices]);
    m_edgeProgram->setColor(model->edgeColor());
    m_edgeProgram->setModelViewMatrix(modelViewMatrix);
    m_edgeProgram->setProjectionMatrix(projectionMatrix);
    m_edgeProgram->setPosition(0, stride);
    glDrawElements(GL_TRIANGLES, model->edgeIndicesCount(), GL_UNSIGNED_SHORT, 0);
    m_edgeProgram->unbind();

#ifdef VPVL_COORDINATE_OPENGL
    glCullFace(GL_BACK);
#else
    glPopMatrix();
    glCullFace(GL_FRONT);
#endif
}

void Renderer::drawModelShadow(const vpvl::PMDModel *model)
{
    return;

    const size_t stride = model->strideSize(vpvl::PMDModel::kVerticesStride);
    const vpvl::PMDModelUserData *modelPrivate = model->userData();

    float modelViewMatrix[16], projectionMatrix[16];
    m_scene->getModelViewMatrix(modelViewMatrix);
    m_scene->getProjectionMatrix(projectionMatrix);

    glDisable(GL_CULL_FACE);
    size_t len = model->vertices().count() * stride;
    glBindBuffer(GL_ARRAY_BUFFER, modelPrivate->vertexBufferObjects[kModelVertices]);
    glBufferData(GL_ARRAY_BUFFER, len, model->verticesPointer(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelPrivate->vertexBufferObjects[kShadowIndices]);
    m_shadowProgram->bind();
    m_shadowProgram->setLightAmbient(m_scene->lightAmbient());
    m_shadowProgram->setLightColor(m_scene->lightColor());
    m_shadowProgram->setLightDiffuse(m_scene->lightDiffuse());
    m_shadowProgram->setLightPosition(m_scene->lightPosition());
    m_shadowProgram->setLightSpecular(m_scene->lightSpecular());
    m_shadowProgram->setModelViewMatrix(modelViewMatrix);
    m_shadowProgram->setProjectionMatrix(projectionMatrix);
    m_shadowProgram->setPosition(0, stride);
    glDrawElements(GL_TRIANGLES, model->indices().count(), GL_UNSIGNED_SHORT, 0);
    m_shadowProgram->unbind();
    glEnable(GL_CULL_FACE);
}

void Renderer::drawModelBones(bool drawSpheres, bool drawLines)
{
    if (m_selected)
        drawModelBones(m_selected, drawSpheres, drawLines);
}

void Renderer::drawModelBones(const vpvl::PMDModel *model, bool drawSpheres, bool drawLines)
{
    const vpvl::BoneList &bones = model->bones();
    btVector3 color;
    uint32_t nBones = bones.count();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();

    for (uint32_t i = 0; i < nBones; i++) {
        const vpvl::Bone *bone = bones[i], *parent = bone->parent();
        vpvl::Bone::Type type = bone->type();
        if (type == vpvl::Bone::kIKTarget && parent && parent->isSimulated())
            continue;
        const btTransform &transform = bone->localTransform();
        if (drawSpheres && type != vpvl::Bone::kInvisible) {
            float scale;
            if (bone->isSimulated()) {
                color.setValue(0.8f, 0.8f, 0.0f);
                scale = 0.1f;
            }
            else {
                switch (type) {
                case vpvl::Bone::kIKDestination:
                    color.setValue(0.7f, 0.2f, 0.2f);
                    scale = 0.25f;
                    break;
                case vpvl::Bone::kUnderIK:
                    color.setValue(0.8f, 0.5f, 0.0f);
                    scale = 0.15f;
                    break;
                case vpvl::Bone::kIKTarget:
                    color.setValue(1.0f, 0.0f, 0.0f);
                    scale = 0.15f;
                    break;
                case vpvl::Bone::kUnderRotate:
                case vpvl::Bone::kTwist:
                case vpvl::Bone::kFollowRotate:
                    color.setValue(0.0f, 0.8f, 0.2f);
                    scale = 0.15f;
                    break;
                default:
                    if (bone->hasMotionIndependency()) {
                        color.setValue(0.0f, 1.0f, 1.0f);
                        scale = 0.25f;
                    } else {
                        color.setValue(0.0f, 0.5f, 1.0f);
                        scale = 0.15f;
                    }
                    break;
                }
            }
            m_debugDrawer->drawSphere(transform.getOrigin(), scale, color);
        }
        if (!drawLines || !parent || type == vpvl::Bone::kIKDestination)
            continue;
        if (type == vpvl::Bone::kInvisible) {
            color.setValue(0.5f, 0.4f, 0.5f);
        }
        else if (bone->isSimulated()) {
            color.setValue(0.7f, 0.7f, 0.0f);
        }
        else if (type == vpvl::Bone::kUnderIK || type == vpvl::Bone::kIKTarget) {
            color.setValue(0.8f, 0.5f, 0.3f);
        }
        else {
            color.setValue(0.5f, 0.6f, 1.0f);
        }
        m_debugDrawer->drawLine(parent->localTransform().getOrigin(), transform.getOrigin(), color);
    }

    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void Renderer::drawBoneTransform(vpvl::Bone *bone)
{
    if (bone) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glPushMatrix();
        btTransform t = bone->localTransform();
        btScalar orthoLen = 1.0f;
        if (bone->hasParent()) {
            btTransform pt = bone->parent()->localTransform();
            orthoLen = btMin(orthoLen, pt.getOrigin().distance(t.getOrigin()));
        }
        m_debugDrawer->drawTransform(t, orthoLen);
        glPopMatrix();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
    }
}

void Renderer::loadAsset(Asset *asset, const std::string &dir)
{
#ifdef VPVL_LINK_ASSIMP
    const aiScene *scene = asset->getScene();
    const uint32_t nMaterials = scene->mNumMaterials;
    AssetUserData *userData = new AssetUserData();
    aiString texturePath;
    std::string path, canonicalized, filename;
    asset->setUserData(userData);
    for (uint32_t i = 0; i < nMaterials; i++) {
        aiMaterial *material = scene->mMaterials[i];
        aiReturn found = AI_SUCCESS;
        GLuint textureID;
        int textureIndex = 0;
        while (found == AI_SUCCESS) {
            found = material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath);
            path = texturePath.data;
            if (userData->textures[path] == 0) {
                canonicalized = m_delegate->toUnicode(reinterpret_cast<const uint8_t *>(CanonicalizePath(path).c_str()));
                filename = dir + "/" + canonicalized;
                if (m_delegate->loadTexture(filename, textureID)) {
                    userData->textures[path] = textureID;
                    m_delegate->log(IDelegate::kLogInfo, "Loaded a texture: %s (ID=%d)", canonicalized.c_str(), textureID);
                }
            }
            textureIndex++;
        }
    }
    aiLoadAssetRecursive(scene, scene->mRootNode, userData, m_delegate);
    m_assets.add(asset);
#else
    (void) asset;
    (void) dir;
#endif
}

void Renderer::unloadAsset(Asset *&asset)
{
#ifdef VPVL_LINK_ASSIMP
    if (asset) {
        const aiScene *scene = asset->getScene();
        const uint32_t nMaterials = scene->mNumMaterials;
        AssetUserData *userData = asset->userData();
        aiString texturePath;
        for (uint32_t i = 0; i < nMaterials; i++) {
            aiMaterial *material = scene->mMaterials[i];
            aiReturn found = AI_SUCCESS;
            GLuint textureID;
            int textureIndex = 0;
            while (found == AI_SUCCESS) {
                found = material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath);
                textureID = userData->textures[texturePath.data];
                glDeleteTextures(1, &textureID);
                userData->textures.erase(texturePath.data);
                textureIndex++;
            }
        }
        aiUnloadAssetRecursive(scene, scene->mRootNode, userData);
        delete userData;
        delete asset;
        m_assets.remove(asset);
        asset = 0;
    }
#else
    (void) asset;
#endif
}

void Renderer::clearSurface()
{
    glViewport(0, 0, m_scene->width(), m_scene->height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::preShadow()
{
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glColorMask(0, 0, 0, 0);
    glDepthMask(0);
    glDisable(GL_DEPTH_TEST);
}

void Renderer::drawShadow()
{
    size_t size = 0;
    vpvl::PMDModel **models = m_scene->getRenderingOrder(size);
    for (size_t i = 0; i < size; i++) {
        vpvl::PMDModel *model = models[i];
        if (model->isVisible()) {
            updateModelBuffer(model);
            drawModelShadow(model);
        }
    }
}

void Renderer::postShadow()
{
    glColorMask(1, 1, 1, 1);
    glDepthMask(1);
    glStencilFunc(GL_EQUAL, 2, ~0);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::drawAssets()
{
    uint32_t nAssets = m_assets.count();
    for (uint32_t i = 0; i < nAssets; i++) {
        aiDrawAsset(m_assets[i], m_scene);
    }
}

void Renderer::drawModels()
{
    size_t size = 0;
    vpvl::PMDModel **models = m_scene->getRenderingOrder(size);
    for (size_t i = 0; i < size; i++) {
        vpvl::PMDModel *model = models[i];
        if (model->isVisible()) {
            drawModel(model);
            drawModelEdge(model);
        }
    }
}

void Renderer::drawSurface()
{
    clearSurface();
    preShadow();
    drawShadow();
    postShadow();
    drawAssets();
    drawModels();
}

}
}
