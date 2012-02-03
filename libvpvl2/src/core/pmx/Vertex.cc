/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2012  hkrn                                    */
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

#include "vpvl2/vpvl2.h"
#include "vpvl2/internal/util.h"

namespace vpvl2
{
namespace pmx
{

#pragma pack(push, 1)

struct VertexUnit {
    float position[3];
    float normal[3];
    float texcoord[2];
};

struct AdditinalUVUnit {
    float value[4];
};

struct Bdef2Unit {
    float weight;
};

struct Bdef4Unit {
    float weight[4];
};

struct SdefUnit {
    float c[3];
    float r0[3];
    float r1[3];
    float weight;
};

#pragma pack(pop)

Vertex::Vertex()
    : m_type(kBdef1),
      m_edge(0)
{
}

Vertex::~Vertex()
{
}

bool Vertex::preparse(uint8_t *&ptr, size_t &rest, Model::DataInfo &info)
{
    size_t size;
    if (!internal::size32(ptr, rest, size)) {
        return false;
    }
    info.verticesPtr = ptr;
    size_t baseSize = sizeof(VertexUnit) + sizeof(AdditinalUVUnit) * info.additionalUVSize;
    for (int i = 0; i < size; i++) {
        if (!internal::validateSize(ptr, baseSize, rest)) {
            return false;
        }
        size_t type;
        /* bone type */
        if (!internal::size8(ptr, rest, type))
            return false;
        size_t boneSize;
        switch (type) {
        case 0: /* BDEF1 */
            boneSize = info.boneIndexSize;
            break;
        case 1: /* BDEF2 */
            boneSize = info.boneIndexSize * 2 + sizeof(Bdef2Unit);
            break;
        case 2: /* BDEF4 */
            boneSize = info.boneIndexSize * 4 + sizeof(Bdef4Unit);
            break;
        case 3: /* SDEF */
            boneSize = info.boneIndexSize * 2 + sizeof(SdefUnit);
            break;
        default: /* unexpected value */
            assert(0);
            return false;
        }
        boneSize += sizeof(float); /* edge */
        if (!internal::validateSize(ptr, boneSize, rest)) {
            return false;
        }
    }
    info.verticesCount = size;
    return rest > 0;
}

void Vertex::read(const uint8_t *data, const Model::DataInfo &info, size_t &size)
{
    uint8_t *ptr = const_cast<uint8_t *>(data), *start = ptr;
    const VertexUnit &vertex = *reinterpret_cast<VertexUnit *>(ptr);
    m_position.setValue(vertex.position[0], vertex.position[1], vertex.position[2]);
    m_normal.setValue(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
    m_texcoord.setValue(vertex.texcoord[0], vertex.texcoord[1], 0);
    ptr += sizeof(vertex);
    int additionalUVSize = info.additionalUVSize;
    for (int i = 0; i < additionalUVSize; i++) {
        const AdditinalUVUnit &uv = *reinterpret_cast<AdditinalUVUnit *>(ptr);
        m_uvs[i].setValue(uv.value[0], uv.value[1], uv.value[2], uv.value[3]);
        ptr += sizeof(uv);
    }
    m_type = static_cast<Type>(*reinterpret_cast<uint8_t *>(ptr));
    ptr += sizeof(uint8_t);
    switch (m_type) {
    case kBdef1: { /* BDEF1 */
        m_bt.bdef1.index = internal::variantIndex(ptr, info.boneIndexSize);
        break;
    }
    case kBdef2: { /* BDEF2 */
        for (int i = 0; i < 2; i++)
            m_bt.bdef4.index[i] = internal::variantIndex(ptr, info.boneIndexSize);
        const Bdef2Unit &unit = *reinterpret_cast<Bdef2Unit *>(ptr);
        m_bt.bdef2.weight = unit.weight;
        ptr += sizeof(Bdef2Unit);
        break;
    }
    case kBdef4: { /* BDEF4 */
        for (int i = 0; i < 4; i++)
            m_bt.bdef4.index[i] = internal::variantIndex(ptr, info.boneIndexSize);
        const Bdef4Unit &unit = *reinterpret_cast<Bdef4Unit *>(ptr);
        for (int i = 0; i < 4; i++)
            m_bt.bdef4.weight[i] = unit.weight[i];
        ptr += sizeof(Bdef4Unit);
        break;
    }
    case kSdef: { /* SDEF */
        for (int i = 0; i < 2; i++)
            m_bt.bdef4.index[i] = internal::variantIndex(ptr, info.boneIndexSize);
        const SdefUnit &unit = *reinterpret_cast<SdefUnit *>(ptr);
        for (int i = 0; i < 3; i++) {
            m_bt.sdef.c[i] = unit.c[i];
            m_bt.sdef.r0[i] = unit.r0[i];
            m_bt.sdef.r1[i] = unit.r1[i];
        }
        m_bt.sdef.weight = unit.weight;
        ptr += sizeof(SdefUnit);
        break;
    }
    default: /* unexpected value */
        assert(0);
        return;
    }
    m_edge = *reinterpret_cast<float *>(ptr);
    ptr += sizeof(m_edge);
    size = ptr - start;
}

void Vertex::write(uint8_t *data) const
{
}

} /* namespace pmx */
} /* namespace vpvl2 */

