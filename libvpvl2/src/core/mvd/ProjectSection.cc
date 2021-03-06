/**

 Copyright (c) 2010-2014  hkrn

 All rights reserved.

 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:

 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
 - Neither the name of the MMDAI project team nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

*/

#include "vpvl2/vpvl2.h"
#include "vpvl2/internal/util.h"

#include "vpvl2/mvd/ProjectKeyframe.h"
#include "vpvl2/mvd/ProjectSection.h"

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{
namespace mvd
{

#pragma pack(push, 1)

struct ProjectSectionHeader {
    int32 reserved;
    int32 sizeOfKeyframe;
    int32 countOfKeyframes;
    int32 reserved2;
};

#pragma pack(pop)

class ProjectSection::PrivateContext : public BaseAnimationTrack {
public:
    PrivateContext()
    {
    }
    ~PrivateContext() {
    }
};

ProjectSection::ProjectSection(Motion *motionRef)
    : BaseSection(motionRef),
      m_context(new ProjectSection::PrivateContext())
{
}

ProjectSection::~ProjectSection()
{
    internal::deleteObject(m_context);
}

bool ProjectSection::preparse(uint8 *&ptr, vsize &rest, Motion::DataInfo &info)
{
    ProjectSectionHeader header;
    if (!internal::validateSize(ptr, sizeof(header), rest)) {
        return false;
    }
    internal::getData(ptr - sizeof(header), header);
    if (!internal::validateSize(ptr, header.reserved2, rest)) {
        return false;
    }
    const int nkeyframes = header.countOfKeyframes;
    const vsize reserved = header.sizeOfKeyframe - ProjectKeyframe::size();
    for (int i = 0; i < nkeyframes; i++) {
        if (!ProjectKeyframe::preparse(ptr, rest, reserved, info)) {
            return false;
        }
    }
    return true;
}

void ProjectSection::read(const uint8 * /* data */)
{
}

void ProjectSection::seek(const IKeyframe::TimeIndex &timeIndex)
{
    saveCurrentTimeIndex(timeIndex);
}

void ProjectSection::write(uint8 * /* data */) const
{
}

vsize ProjectSection::estimateSize() const
{
    return 0;
}

vsize ProjectSection::countKeyframes() const
{
    return m_context->keyframes.count();
}

void ProjectSection::update()
{
    updateKeyframes(m_context->keyframes);
}

void ProjectSection::addKeyframe(IKeyframe *keyframe)
{
    m_context->keyframes.append(keyframe);
}

void ProjectSection::removeKeyframe(IKeyframe *keyframe)
{
    m_context->keyframes.remove(keyframe);
}

void ProjectSection::deleteKeyframe(IKeyframe *&keyframe)
{
    removeKeyframe(keyframe);
    internal::deleteObject(keyframe);
}

void ProjectSection::getKeyframes(const IKeyframe::TimeIndex & /* timeIndex */,
                                  const IKeyframe::LayerIndex & /* layerIndex */,
                                  Array<IKeyframe *> & /* keyframes */) const
{
}

void ProjectSection::getAllKeyframes(Array<IKeyframe *> &keyframes) const
{
    keyframes.copy(m_context->keyframes);
}

void ProjectSection::setAllKeyframes(const Array<IKeyframe *> &value)
{
    release();
    m_context = new PrivateContext();
    const int nkeyframes = value.count();
    for (int i = 0; i < nkeyframes; i++) {
        IKeyframe *keyframe = value[i];
        if (keyframe && keyframe->type() == IKeyframe::kProjectKeyframe) {
            addKeyframe(keyframe);
        }
    }
}

void ProjectSection::createFirstKeyframeUnlessFound()
{
    if (!findKeyframe(0, 0)) {
        ProjectKeyframe *keyframe = m_context->keyframes.append(new ProjectKeyframe(m_motionRef));
        keyframe->setGravityDirection(Vector3(0, -9.8, 0));
        keyframe->setGravityFactor(1.0);
        keyframe->setLayerIndex(0);
        keyframe->setShadowDepth(0);
        keyframe->setShadowDistance(0);
        keyframe->setShadowMode(0);
        keyframe->setTimeIndex(0);
        update();
    }
}

IKeyframe::LayerIndex ProjectSection::countLayers() const
{
    return 1;
}

IProjectKeyframe *ProjectSection::findKeyframe(const IKeyframe::TimeIndex &timeIndex,
                                               const IKeyframe::LayerIndex &layerIndex) const
{
    const PrivateContext::KeyframeCollection &keyframes = m_context->keyframes;
    const int nkeyframes = keyframes.count();
    for (int i = 0; i < nkeyframes; i++) {
        ProjectKeyframe *keyframe = reinterpret_cast<ProjectKeyframe *>(keyframes[i]);
        if (keyframe->timeIndex() == timeIndex && keyframe->layerIndex() == layerIndex) {
            return keyframe;
        }
    }
    return 0;
}

IProjectKeyframe *ProjectSection::findKeyframeAt(int index) const
{
    const PrivateContext::KeyframeCollection &keyframes = m_context->keyframes;
    if (internal::checkBound(index, 0, keyframes.count())) {
        ProjectKeyframe *keyframe = reinterpret_cast<ProjectKeyframe *>(keyframes[index]);
        return keyframe;
    }
    return 0;
}

} /* namespace mvd */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */
