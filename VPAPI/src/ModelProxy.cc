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

#include <vpvl2/vpvl2.h>
#include <vpvl2/extensions/XMLProject.h>
#include <vpvl2/extensions/qt/String.h>

#include <QtCore>
#include <QVector3D>

#include "BoneRefObject.h"
#include "IKConstraintRefObject.h"
#include "JointRefObject.h"
#include "LabelRefObject.h"
#include "MaterialRefObject.h"
#include "ModelProxy.h"
#include "MorphRefObject.h"
#include "MotionProxy.h"
#include "ProjectProxy.h"
#include "RigidBodyRefObject.h"
#include "Util.h"
#include "WorldProxy.h"
#include "VertexRefObject.h"

using namespace vpvl2;
using namespace vpvl2::extensions;
using namespace vpvl2::extensions::qt;

ModelProxy::ModelProxy(ProjectProxy *project,
                       IModel *model,
                       const QUuid &uuid,
                       const QUrl &fileUrl,
                       const QUrl &faviconUrl,
                       QUndoStack *undoStackRef)
    : QObject(project),
      m_parentProjectRef(project),
      m_childMotionRef(0),
      m_model(model),
      m_uuid(uuid),
      m_fileUrl(fileUrl),
      m_faviconUrl(faviconUrl),
      m_undoStackRef(undoStackRef),
      m_targetMorphRef(0),
      m_boneAxisType(AxisX),
      m_boneTransformType(LocalTransform),
      m_language(project->language()),
      m_baseY(0),
      m_moving(false),
      m_dirty(false)
{
    Q_ASSERT(m_parentProjectRef);
    Q_ASSERT(m_undoStackRef);
    Q_ASSERT(!m_model.isNull());
    Q_ASSERT(!m_uuid.isNull());
    connect(m_parentProjectRef, &ProjectProxy::languageChanged, this, &ModelProxy::resetLanguage);
    connect(this, &ModelProxy::boneDidSelect, this, &ModelProxy::firstTargetBoneChanged);
    connect(this, &ModelProxy::morphDidSelect, this, &ModelProxy::firstTargetMorphChanged);
    connect(this, &ModelProxy::modelDidRefresh, this, &ModelProxy::translationChanged);
    connect(this, &ModelProxy::modelDidRefresh, this, &ModelProxy::orientationChanged);
    connect(this, &ModelProxy::modelDidRefresh, this, &ModelProxy::scaleFactorChanged);
    connect(this, &ModelProxy::modelDidRefresh, this, &ModelProxy::opacityChanged);
    connect(this, &ModelProxy::modelDidRefresh, this, &ModelProxy::edgeWidthChanged);
    connect(this, &ModelProxy::modelDidRefresh, this, &ModelProxy::visibleChanged);
    connect(this, &ModelProxy::languageChanged, this, &ModelProxy::nameChanged);
    connect(this, &ModelProxy::languageChanged, this, &ModelProxy::commentChanged);
    VPVL2_VLOG(1, "The model " << uuid.toString().toStdString() << " a.k.a " << name().toStdString() << " is added");
}

ModelProxy::~ModelProxy()
{
    VPVL2_VLOG(1, "The model " << uuid().toString().toStdString() << " a.k.a " << name().toStdString() << " will be deleted");
    qDeleteAll(m_allBones);
    m_allBones.clear();
    qDeleteAll(m_allMorphs);
    m_allMorphs.clear();
    qDeleteAll(m_allLabels);
    m_allLabels.clear();
    qDeleteAll(m_allMaterials);
    m_allMaterials.clear();
    qDeleteAll(m_allVertices);
    m_allVertices.clear();
    qDeleteAll(m_allRigidBodies);
    m_allRigidBodies.clear();
    qDeleteAll(m_allJoints);
    m_allJoints.clear();
    m_parentProjectRef = 0;
    m_undoStackRef = 0;
    m_childMotionRef = 0;
    m_targetMorphRef = 0;
    m_baseY = -1;
}

bool ModelProxy::save(const QUrl &fileUrl)
{
    bool result = false;
    if (fileUrl.fileName().endsWith(".json")) {
        result = saveJson(fileUrl);
    }
    else {
        QFile file(fileUrl.toLocalFile());
        if (file.open(QFile::WriteOnly)) {
            QByteArray bytes;
            vsize written = 0;
            const IString::Codec &codec = m_model->encodingType();
            m_model->setEncodingType(IString::kUTF8);
            bytes.resize(m_model->estimateSize());
            m_model->save(reinterpret_cast<uint8 *>(bytes.data()), written);
            Q_ASSERT(m_model->estimateSize() == written);
            file.write(bytes.constData(), written);
            file.close();
            m_model->setEncodingType(codec);
            result = true;
        }
        else {
            qWarning() << file.errorString();
            result = false;
        }
    }
    return result;
}

bool ModelProxy::saveJson(const QUrl &fileUrl) const
{
    QFile file(fileUrl.toLocalFile());
    if (file.open(QFile::WriteOnly)) {
        QJsonDocument document(toJson().toObject());
        file.write(document.toJson());
        file.close();
        return true;
    }
    else {
        qWarning() << file.errorString();
        return false;
    }
}

QJsonValue ModelProxy::toJson() const
{
    QJsonObject v;
    v.insert("uuid", uuid().toString());
    v.insert("name", name());
    v.insert("comment", comment());
    v.insert("version", version());
    v.insert("visible", isVisible());
    v.insert("scaleFactor", scaleFactor());
    v.insert("opacity", opacity());
    v.insert("translation", Util::toJson(translation()));
    v.insert("orientation", Util::toJson(orientation()));
    QJsonArray vertices;
    foreach (VertexRefObject *vertex, m_allVertices) {
        vertices.append(vertex->toJson());
    }
    v.insert("vertices", vertices);
    QJsonArray materials;
    foreach (MaterialRefObject *material, m_allMaterials) {
        materials.append(material->toJson());
    }
    v.insert("materials", materials);
    QJsonArray bones;
    foreach (BoneRefObject *bone, m_allBones) {
        bones.append(bone->toJson());
    }
    v.insert("bones", bones);
    QJsonArray labels;
    foreach (LabelRefObject *label, m_allLabels) {
        labels.append(label->toJson());
    }
    v.insert("labels", labels);
    QJsonArray morphs;
    foreach (MorphRefObject *morph, m_allMorphs) {
        morphs.append(morph->toJson());
    }
    v.insert("morphs", morphs);
    QJsonArray rigidBodies;
    foreach (RigidBodyRefObject *body, m_allRigidBodies) {
        rigidBodies.append(body->toJson());
    }
    v.insert("rigidBodies", rigidBodies);
    QJsonArray joints;
    foreach (JointRefObject *joint, m_allJoints) {
        joints.append(joint->toJson());
    }
    v.insert("joints", joints);
    return v;
}

void ModelProxy::initialize(bool all)
{
    Q_ASSERT(m_model);
    Array<ILabel *> labelRefs;
    m_model->getLabelRefs(labelRefs);
    initializeAllBones(labelRefs);
    initializeAllMorphs(labelRefs, all);
    qSort(m_allLabels.begin(), m_allLabels.end(), Util::LessThan());
    if (all) {
        initializeAllJoints();
        initializeAllMaterials();
        initializeAllRigidBodies();
        initializeAllVertices();
    }
    setDirty(false);
}

void ModelProxy::addBindingModel(ModelProxy *value)
{
    m_bindingModels.append(value);
}

void ModelProxy::removeBindingModel(ModelProxy *value)
{
    m_bindingModels.removeOne(value);
}

void ModelProxy::releaseBindings()
{
    setParentBindingBone(0);
    setParentBindingModel(0);
    foreach (ModelProxy *modelProxy, m_bindingModels) {
        modelProxy->setParentBindingBone(0);
        modelProxy->setParentBindingModel(0);
    }
    m_bindingModels.clear();
}

void ModelProxy::renameObject(QObject *object, const QString &newName)
{
    Q_ASSERT(m_model);
    if (MaterialRefObject *material = qobject_cast<MaterialRefObject *>(object)) {
        m_name2MaterialRefs.remove(material->name());
        m_name2MaterialRefs.insert(newName, material);
    }
    else if (BoneRefObject *bone = qobject_cast<BoneRefObject *>(object)) {
        m_name2BoneRefs.remove(bone->name());
        m_name2BoneRefs.insert(newName, bone);
    }
    else if (MorphRefObject *morph = qobject_cast<MorphRefObject *>(object)) {
        m_name2MorphRefs.remove(morph->name());
        m_name2MorphRefs.insert(newName, morph);
    }
    else if (LabelRefObject *label = qobject_cast<LabelRefObject *>(object)) {
        Q_UNUSED(label);
        /* FIXME: implement this */
    }
    else if (RigidBodyRefObject *body = qobject_cast<RigidBodyRefObject *>(object)) {
        m_name2RigidBodyRefs.remove(body->name());
        m_name2RigidBodyRefs.insert(newName, body);
    }
    else if (JointRefObject *joint = qobject_cast<JointRefObject *>(object)) {
        m_name2JointRefs.remove(bone->name());
        m_name2JointRefs.insert(newName, joint);
    }
}

void ModelProxy::selectOpaqueObject(QObject *value)
{
    if (BoneRefObject *bone = qobject_cast<BoneRefObject *>(value)) {
        selectBone(bone);
    }
    else if (MorphRefObject *morph = qobject_cast<MorphRefObject *>(value)) {
        setFirstTargetMorph(morph);
    }
}

void ModelProxy::selectBone(BoneRefObject *value)
{
    if (value && m_uuid2BoneRefs.contains(value->uuid())) {
        m_targetBoneRefs.clear();
        m_targetBoneRefs.append(value);
    }
    else if (!m_targetBoneRefs.isEmpty()) {
        m_targetBoneRefs.clear();
        value = 0;
    }
    emit boneDidSelect(value);
}

void ModelProxy::beginTransform(qreal startY)
{
    Q_ASSERT(!m_moving);
    saveTransformState();
    m_baseY = startY;
    m_moving = true;
    emit transformDidBegin();
}

void ModelProxy::translate(qreal value)
{
    Q_ASSERT(m_parentProjectRef);
    if (isMoving()) {
        const XMLProject *project = m_parentProjectRef->projectInstanceRef();
        const ICamera *camera = project->cameraRef();
        const Matrix3x3 &cameraTransformMatrix = camera->modelViewTransform().getBasis();
        Vector3 newTranslation(kZeroV3), translation;
        qreal delta = (value - m_baseY) * 0.05;
        switch (m_boneAxisType) {
        case AxisX:
            newTranslation.setX(delta);
            break;
        case AxisY:
            newTranslation.setY(delta);
            break;
        case AxisZ:
            newTranslation.setZ(delta);
            break;
        }
        foreach (BoneRefObject *boneRef, m_targetBoneRefs) {
            if (boneRef->isMovable()) {
                if (m_transformState.contains(boneRef)) {
                    const InternalTransform &t = m_transformState.value(boneRef);
                    translation = t.first;
                }
                else {
                    translation = boneRef->rawLocalTranslation();
                }
                switch (m_boneTransformType) {
                case LocalTransform: {
                    Transform transform(boneRef->rawLocalOrientation(), translation);
                    boneRef->setRawLocalTranslation(transform * newTranslation);
                    break;
                }
                case GlobalTransform: {
                    boneRef->setRawLocalTranslation(translation + newTranslation);
                    break;
                }
                case ViewTransform: {
                    Transform transform(boneRef->rawLocalOrientation(), translation);
                    Vector3 newDelta = kZeroV3;
                    newDelta += cameraTransformMatrix[0] * newTranslation.x();
                    newDelta += cameraTransformMatrix[1] * newTranslation.y();
                    newDelta += cameraTransformMatrix[2] * newTranslation.z();
                    boneRef->setRawLocalTranslation(transform * newDelta);
                    break;
                }
                default:
                    break;
                }
            }
        }
        emit targetBonesDidTranslate();
    }
}

void ModelProxy::rotate(qreal angle)
{
    Q_ASSERT(m_parentProjectRef);
    if (isMoving()) {
        const XMLProject *project = m_parentProjectRef->projectInstanceRef();
        const ICamera *camera = project->cameraRef();
        const Matrix3x3 &cameraTransformMatrix = camera->modelViewTransform().getBasis();
        Quaternion rotation;
        Scalar radian = btRadians((angle - m_baseY) * 0.5);
        foreach (BoneRefObject *boneRef, m_targetBoneRefs) {
            if (boneRef->isRotateable()) {
                if (m_transformState.contains(boneRef)) {
                    const InternalTransform &t = m_transformState.value(boneRef);
                    rotation = t.second;
                }
                else {
                    rotation = boneRef->rawLocalOrientation();
                }
                Quaternion delta = Quaternion::getIdentity();
                if (m_transformState.contains(boneRef)) {
                    const InternalTransform &t = m_transformState.value(boneRef);
                    delta = t.second;
                }
                if (boneRef->isFixedAxisEnabled()) {
                    delta.setRotation(boneRef->rawFixedAxis(), btRadians(angle - m_baseY));
                }
                else {
                    Vector3 axisX(1, 0, 0), axisY(0, 1, 0), axisZ(0, 0, 1);
                    switch (m_boneTransformType) {
                    case LocalTransform: {
                        Matrix3x3 axes;
                        if (boneRef->isLocalAxesEnabled()) {
                            boneRef->getLocalAxes(axes);
                            axisX = axes[0];
                            axisY = axes[1];
                            axisZ = axes[2];
                        }
                        else {
                            axes.setRotation(rotation);
                            axisX = axes[0] * axisX;
                            axisY = axes[1] * axisY;
                            axisZ = axes[2] * axisZ;
                        }
                        break;
                    }
                    case GlobalTransform: {
                        break;
                    }
                    case ViewTransform: {
                        axisX = cameraTransformMatrix.getRow(0);
                        axisY = cameraTransformMatrix.getRow(1);
                        axisZ = cameraTransformMatrix.getRow(2);
                        break;
                    }
                    default:
                        break;
                    }
                    switch (m_boneAxisType) {
                    case AxisX:
                        delta.setRotation(axisX, -radian);
                        break;
                    case AxisY:
                        delta.setRotation(axisY, -radian);
                        break;
                    case AxisZ:
                        delta.setRotation(axisZ, radian);
                        break;
                    }
                }
                boneRef->setRawLocalOrientation(rotation * delta);
            }
        }
        emit targetBonesDidRotate();
    }
}

void ModelProxy::discardTransform()
{
    Q_ASSERT(m_moving);
    clearTransformState();
    m_baseY = 0;
    m_moving = false;
    emit transformDidDiscard();
}

void ModelProxy::commitTransform()
{
    Q_ASSERT(m_moving);
    clearTransformState();
    m_baseY = 0;
    m_moving = false;
    emit transformDidCommit();
}

void ModelProxy::resetTargets()
{
    m_targetBoneRefs.clear();
    m_targetMorphRef = 0;
    boneDidSelect(0);
    morphDidSelect(0);
}

void ModelProxy::release()
{
    Q_ASSERT(m_parentProjectRef);
    if (!m_parentProjectRef->resolveModelProxy(data())) {
        VPVL2_VLOG(1, uuid().toString().toStdString() << " a.k.a " << name().toStdString() << " is scheduled to be delete from ModelProxy and will be deleted");
        m_parentProjectRef->world()->leaveWorld(this);
        deleteLater();
    }
}

void ModelProxy::refresh()
{
    emit modelDidRefresh();
}

BoneRefObject *ModelProxy::resolveBoneRef(const IBone *value) const
{
    return m_bone2Refs.value(value);
}

BoneRefObject *ModelProxy::findBoneByName(const QString &name) const
{
    return m_name2BoneRefs.value(name);
}

BoneRefObject *ModelProxy::findBoneByUuid(const QUuid &uuid) const
{
    return m_uuid2BoneRefs.value(uuid);
}

MorphRefObject *ModelProxy::resolveMorphRef(const IMorph *value) const
{
    return m_morph2Refs.value(value);
}

MorphRefObject *ModelProxy::findMorphByName(const QString &name) const
{
    return m_name2MorphRefs.value(name);
}

MorphRefObject *ModelProxy::findMorphByUuid(const QUuid &uuid) const
{
    return m_uuid2MorphRefs.value(uuid);
}

QList<QObject *> ModelProxy::findMorphsByCategory(int type) const
{
    QList<QObject *> morphs;
    MorphRefObject::Category category = static_cast<MorphRefObject::Category>(type);
    foreach (MorphRefObject *morph, m_allMorphs) {
        if (morph->category() == category) {
            morphs.append(morph);
        }
    }
    return morphs;
}

MaterialRefObject *ModelProxy::resolveMaterialRef(const vpvl2::IMaterial *value) const
{
    return m_material2Refs.value(value);
}

MaterialRefObject *ModelProxy::findMaterialByName(const QString &name) const
{
    return m_name2MaterialRefs.value(name);
}

MaterialRefObject *ModelProxy::findMaterialByUuid(const QUuid &uuid) const
{
    return m_uuid2MaterialRefs.value(uuid);
}

VertexRefObject *ModelProxy::resolveVertexRef(const vpvl2::IVertex *value) const
{
    return m_vertex2Refs.value(value);
}

VertexRefObject *ModelProxy::findVertexByUuid(const QUuid &uuid) const
{
    return m_uuid2VertexRefs.value(uuid);
}

RigidBodyRefObject *ModelProxy::resolveRigidBodyRef(const vpvl2::IRigidBody *value) const
{
    return m_rigidBody2Refs.value(value);
}

RigidBodyRefObject *ModelProxy::findRigidBodyByName(const QString &name) const
{
    return m_name2RigidBodyRefs.value(name);
}

RigidBodyRefObject *ModelProxy::findRigidBodyByUuid(const QUuid &uuid) const
{
    return m_uuid2RigidBodyRefs.value(uuid);
}

JointRefObject *ModelProxy::resolveJointRef(const vpvl2::IJoint *value) const
{
    return m_joint2Refs.value(value);
}

JointRefObject *ModelProxy::findJointByName(const QString &name) const
{
    return m_name2JointRefs.value(name);
}

JointRefObject *ModelProxy::findJointByUuid(const QUuid &uuid) const
{
    return m_uuid2JointRefs.value(uuid);
}

VertexRefObject *ModelProxy::createVertex()
{
    Q_ASSERT(m_model);
    QScopedPointer<IVertex> vertex(m_model->createVertex());
    QScopedPointer<VertexRefObject> vertexRef(new VertexRefObject(this, vertex.data(), QUuid::createUuid()));
    initializeAllVertices();
    m_allVertices.append(vertexRef.data());
    m_uuid2VertexRefs.insert(vertexRef->uuid(), vertexRef.data());
    m_vertex2Refs.insert(vertex.data(), vertexRef.data());
    m_model->addVertex(vertex.take());
    emit allVerticesChanged();
    return vertexRef.take();
}

MaterialRefObject *ModelProxy::createMaterial()
{
    Q_ASSERT(m_model);
    QScopedPointer<IMaterial> material(m_model->createMaterial());
    QScopedPointer<MaterialRefObject> materialRef(new MaterialRefObject(this, material.data(), QUuid::createUuid()));
    initializeAllMaterials();
    m_allMaterials.append(materialRef.data());
    m_uuid2MaterialRefs.insert(materialRef->uuid(), materialRef.data());
    m_material2Refs.insert(material.data(), materialRef.data());
    m_model->addMaterial(material.take());
    emit allMaterialsChanged();
    return materialRef.take();
}

BoneRefObject *ModelProxy::createBone()
{
    Q_ASSERT(m_model);
    QScopedPointer<IBone> bone(m_model->createBone());
    QScopedPointer<BoneRefObject> boneRef(new BoneRefObject(this, bone.data(), QUuid::createUuid()));
    m_allBones.append(boneRef.data());
    m_uuid2BoneRefs.insert(boneRef->uuid(), boneRef.data());
    m_bone2Refs.insert(bone.data(), boneRef.data());
    m_model->addBone(bone.take());
    emit allBonesChanged();
    return boneRef.take();
}

MorphRefObject *ModelProxy::createMorph()
{
    Q_ASSERT(m_model);
    QScopedPointer<IMorph> morph(m_model->createMorph());
    QScopedPointer<MorphRefObject> morphRef(new MorphRefObject(this, morph.data(), QUuid::createUuid()));
    m_allMorphs.append(morphRef.data());
    m_uuid2MorphRefs.insert(morphRef->uuid(), morphRef.data());
    m_morph2Refs.insert(morph.data(), morphRef.data());
    m_model->addMorph(morph.take());
    emit allMorphsChanged();
    return morphRef.take();
}

LabelRefObject *ModelProxy::createLabel()
{
    Q_ASSERT(m_model);
    QScopedPointer<ILabel> label(m_model->createLabel());
    QScopedPointer<LabelRefObject> labelRef(new LabelRefObject(this, label.data()));
    m_allLabels.append(labelRef.data());
    m_model->addLabel(label.take());
    emit allLabelsChanged();
    return labelRef.take();
}

RigidBodyRefObject *ModelProxy::createRigidBody()
{
    Q_ASSERT(m_model);
    QScopedPointer<IRigidBody> body(m_model->createRigidBody());
    QScopedPointer<RigidBodyRefObject> bodyRef(new RigidBodyRefObject(this, body.data(), QUuid::createUuid()));
    initializeAllRigidBodies();
    m_allRigidBodies.append(bodyRef.data());
    m_uuid2RigidBodyRefs.insert(bodyRef->uuid(), bodyRef.data());
    m_rigidBody2Refs.insert(body.data(), bodyRef.data());
    m_model->addRigidBody(body.take());
    emit allRigidBodiesChanged();
    return bodyRef.take();
}

JointRefObject *ModelProxy::createJoint()
{
    Q_ASSERT(m_model);
    QScopedPointer<IJoint> joint(m_model->createJoint());
    QScopedPointer<JointRefObject> jointRef(new JointRefObject(this, joint.data(), QUuid::createUuid()));
    initializeAllJoints();
    m_allJoints.append(jointRef.data());
    m_uuid2JointRefs.insert(jointRef->uuid(), jointRef.data());
    m_joint2Refs.insert(joint.data(), jointRef.data());
    m_model->addJoint(joint.take());
    emit allJointsChanged();
    return jointRef.take();
}

QObject *ModelProxy::createObject(ObjectType type)
{
    Q_ASSERT(m_model);
    QObject *object = 0;
    switch (type) {
    case Vertex: {
        object = createVertex();
        break;
    }
    case Material: {
        object = createMaterial();
        break;
    }
    case Bone: {
        object = createBone();
        break;
    }
    case Morph: {
        object = createMorph();
        break;
    }
    case Label: {
        object = createLabel();
        break;
    }
    case RigidBody: {
        object = createRigidBody();
        break;
    }
    case Joint: {
        object = createJoint();
        break;
    }
    case SoftBody: {
        break;
    }
    default:
        break;
    }
    return object;
}

bool ModelProxy::removeVertex(VertexRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    initializeAllVertices();
    m_model->removeVertex(value->data());
    m_vertex2Refs.remove(value->data());
    m_uuid2VertexRefs.remove(value->uuid());
    if (m_allVertices.removeOne(value)) {
        emit allVerticesChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeMaterial(MaterialRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    initializeAllMaterials();
    m_model->removeMaterial(value->data());
    m_material2Refs.remove(value->data());
    m_uuid2MaterialRefs.remove(value->uuid());
    if (m_allMaterials.removeOne(value)) {
        emit allMaterialsChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeBone(BoneRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    m_model->removeBone(value->data());
    m_bone2Refs.remove(value->data());
    m_uuid2BoneRefs.remove(value->uuid());
    if (m_allBones.removeOne(value)) {
        emit allBonesChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeMorph(MorphRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    m_model->removeMorph(value->data());
    m_morph2Refs.remove(value->data());
    m_uuid2MorphRefs.remove(value->uuid());
    if (m_allMorphs.removeOne(value)) {
        emit allMorphsChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeLabel(LabelRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    m_model->removeLabel(value->data());
    if (m_allLabels.removeOne(value)) {
        emit allLabelsChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeRigidBody(RigidBodyRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    initializeAllRigidBodies();
    m_model->removeRigidBody(value->data());
    m_rigidBody2Refs.remove(value->data());
    m_uuid2RigidBodyRefs.remove(value->uuid());
    if (m_allRigidBodies.removeOne(value)) {
        emit allRigidBodiesChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeJoint(JointRefObject *value)
{
    Q_ASSERT(m_model);
    Q_ASSERT(value);
    initializeAllJoints();
    m_model->removeJoint(value->data());
    m_joint2Refs.remove(value->data());
    m_uuid2JointRefs.remove(value->uuid());
    if (m_allJoints.removeOne(value)) {
        emit allJointsChanged();
        return true;
    }
    return false;
}

bool ModelProxy::removeObject(QObject *value)
{
    Q_ASSERT(m_model);
    if (VertexRefObject *vertex = qobject_cast<VertexRefObject *>(value)) {
        return removeVertex(vertex);
    }
    else if (MaterialRefObject *material = qobject_cast<MaterialRefObject *>(value)) {
        return removeMaterial(material);
    }
    else if (BoneRefObject *bone = qobject_cast<BoneRefObject *>(value)) {
        return removeBone(bone);
    }
    else if (MorphRefObject *morph = qobject_cast<MorphRefObject *>(value)) {
        return removeMorph(morph);
    }
    else if (LabelRefObject *label = qobject_cast<LabelRefObject *>(value)) {
        return removeLabel(label);
    }
    else if (RigidBodyRefObject *body = qobject_cast<RigidBodyRefObject *>(value)) {
        return removeRigidBody(body);
    }
    else if (JointRefObject *joint = qobject_cast<JointRefObject *>(value)) {
        return removeJoint(joint);
    }
    return false;
}

bool ModelProxy::deleteObject(QObject *value)
{
    if (removeObject(value)) {
        value->deleteLater();
        return true;
    }
    return false;
}

IModel *ModelProxy::data() const
{
    return m_model.data();
}

ProjectProxy *ModelProxy::parentProject() const
{
    return m_parentProjectRef;
}

QUndoStack *ModelProxy::undoStack() const
{
    return m_undoStackRef;
}

ModelProxy *ModelProxy::parentBindingModel() const
{
    Q_ASSERT(m_parentProjectRef);
    return m_parentProjectRef->resolveModelProxy(data()->parentModelRef());
}

void ModelProxy::setParentBindingModel(ModelProxy *value)
{
    if (value != parentBindingModel()) {
        if (value) {
            value->addBindingModel(this);
            data()->setParentModelRef(value->data());
        }
        else {
            parentBindingModel()->removeBindingModel(this);
            data()->setParentModelRef(0);
        }
        emit parentBindingModelChanged();
    }
}

MotionProxy *ModelProxy::childMotion() const
{
    return m_childMotionRef;
}

void ModelProxy::setChildMotion(MotionProxy *value, bool emitSignal)
{
    if (value && m_childMotionRef != value) {
        value->data()->setParentModelRef(m_model.data());
        m_childMotionRef = value;
        emit childMotionChanged();
    }
    else if (m_childMotionRef) {
        m_childMotionRef = 0;
        if (emitSignal) {
            emit childMotionChanged();
        }
    }
}

BoneRefObject *ModelProxy::parentBindingBone() const
{
    ModelProxy *parentModelRef = parentBindingModel();
    return parentModelRef ? parentModelRef->resolveBoneRef(data()->parentBoneRef()) : 0;
}

void ModelProxy::setParentBindingBone(BoneRefObject *value)
{
    if (value != parentBindingBone()) {
        data()->setParentBoneRef(value ? value->data() : 0);
        emit parentBindingBoneChanged();
    }
}

QUuid ModelProxy::uuid() const
{
    Q_ASSERT(!m_uuid.isNull());
    return m_uuid;
}

QUrl ModelProxy::fileUrl() const
{
    return m_fileUrl;
}

QUrl ModelProxy::faviconUrl() const
{
    return m_faviconUrl;
}

ModelProxy::VersionType ModelProxy::version() const
{
    Q_ASSERT(m_model);
    qreal version = m_model->version();
    if (qFuzzyCompare(version, 2.1)) {
        return PMX_2_1;
    }
    else if (qFuzzyCompare(version, 2.0)) {
        return PMX_2_0;
    }
    else if (qFuzzyCompare(version, 1.0)) {
        return PMD_1_0;
    }
    else {
        return PMD_1_0;
    }
}

void ModelProxy::setVersion(const VersionType &value)
{
    Q_ASSERT(m_model);
    if (version() != value) {
        switch (value) {
        case PMX_2_1:
            m_model->setVersion(2.1);
            break;
        case PMX_2_0:
            m_model->setVersion(2.0);
            break;
        case PMD_1_0:
            m_model->setVersion(1.0);
            break;
        }
        markDirty();
        emit versionChanged();
    }
}

QString ModelProxy::name() const
{
    Q_ASSERT(m_model);
    const IString *name = m_model->name(static_cast<IEncoding::LanguageType>(language()));
    return Util::toQString((name && name->size() > 0) ? name : m_model->name(IEncoding::kDefaultLanguage));
}

void ModelProxy::setName(const QString &value)
{
    Q_ASSERT(m_model);
    if (name() != value) {
        QScopedPointer<IString> s(String::create(value.toStdString()));
        m_model->setName(s.data(), static_cast<IEncoding::LanguageType>(language()));
        markDirty();
        emit nameChanged();
    }
}

QString ModelProxy::comment() const
{
    Q_ASSERT(m_model);
    const IString *comment = m_model->comment(static_cast<IEncoding::LanguageType>(language()));
    return Util::toQString((comment && comment->size() > 0) ? comment : m_model->comment(IEncoding::kDefaultLanguage));
}

void ModelProxy::setComment(const QString &value)
{
    Q_ASSERT(m_model);
    if (comment() != value) {
        QScopedPointer<IString> s(String::create(value.toStdString()));
        m_model->setComment(s.data(), static_cast<IEncoding::LanguageType>(language()));
        markDirty();
        emit commentChanged();
    }
}

ModelProxy::EncodingType ModelProxy::encodingType() const
{
    Q_ASSERT(m_model);
    return static_cast<EncodingType>(m_model->encodingType());
}

void ModelProxy::setEncodingType(EncodingType value)
{
    Q_ASSERT(m_model);
    if (encodingType() != value) {
        m_model->setEncodingType(static_cast<IString::Codec>(value));
        markDirty();
        emit encodingTypeChanged();
    }
}

QQmlListProperty<LabelRefObject> ModelProxy::allLabels()
{
    return QQmlListProperty<LabelRefObject>(this, m_allLabels);
}

QQmlListProperty<BoneRefObject> ModelProxy::allBones()
{
    return QQmlListProperty<BoneRefObject>(this, m_allBones);
}

QQmlListProperty<BoneRefObject> ModelProxy::targetBones()
{
    return QQmlListProperty<BoneRefObject>(this, m_targetBoneRefs);
}

QQmlListProperty<MorphRefObject> ModelProxy::allMorphs()
{
    return QQmlListProperty<MorphRefObject>(this, m_allMorphs);
}

QQmlListProperty<MaterialRefObject> ModelProxy::allMaterials()
{
    initializeAllMaterials();
    return QQmlListProperty<MaterialRefObject>(this, m_allMaterials);
}

QQmlListProperty<VertexRefObject> ModelProxy::allVertices()
{
    initializeAllVertices();
    return QQmlListProperty<VertexRefObject>(this, m_allVertices);
}

QQmlListProperty<RigidBodyRefObject> ModelProxy::allRigidBodies()
{
    initializeAllRigidBodies();
    return QQmlListProperty<RigidBodyRefObject>(this, m_allRigidBodies);
}

QQmlListProperty<JointRefObject> ModelProxy::allJoints()
{
    initializeAllJoints();
    return QQmlListProperty<JointRefObject>(this, m_allJoints);
}

QQmlListProperty<IKConstraintRefObject> ModelProxy::allIKConstraints()
{
    initializeAllIKConstraints();
    return QQmlListProperty<IKConstraintRefObject>(this, m_allIKConstraints);
}

QList<BoneRefObject *> ModelProxy::allTargetBones() const
{
    return m_targetBoneRefs;
}

BoneRefObject *ModelProxy::firstTargetBone() const
{
    return m_targetBoneRefs.isEmpty() ? 0 : m_targetBoneRefs.first();
}

MorphRefObject *ModelProxy::firstTargetMorph() const
{
    return m_targetMorphRef;
}

void ModelProxy::setFirstTargetMorph(MorphRefObject *value)
{
    if (value && value != m_targetMorphRef && m_uuid2MorphRefs.contains(value->uuid())) {
        m_targetMorphRef = value;
        emit morphDidSelect(value);
    }
    else if (!value && m_targetMorphRef) {
        m_targetMorphRef = 0;
        emit morphDidSelect(0);
    }
}

ModelProxy::AxisType ModelProxy::axisType() const
{
    return m_boneAxisType;
}

void ModelProxy::setAxisType(AxisType value)
{
    if (m_boneAxisType != value) {
        m_boneAxisType = value;
        emit axisTypeChanged();
    }
}

ModelProxy::TransformType ModelProxy::transformType() const
{
    return m_boneTransformType;
}

void ModelProxy::setTransformType(TransformType value)
{
    if (m_boneTransformType != value) {
        m_boneTransformType = value;
        emit transformTypeChanged();
    }
}

ProjectProxy::LanguageType ModelProxy::language() const
{
    Q_ASSERT(m_parentProjectRef);
    return m_language != ProjectProxy::DefaultLauguage ? m_language : m_parentProjectRef->language();
}

void ModelProxy::setLanguage(ProjectProxy::LanguageType value)
{
    if (value != m_language) {
        m_language = value;
        emit languageChanged();
    }
}

QVector3D ModelProxy::translation() const
{
    Q_ASSERT(m_model);
    return Util::fromVector3(m_model->worldTranslation());
}

void ModelProxy::setTranslation(const QVector3D &value)
{
    Q_ASSERT(m_model);
    if (!qFuzzyCompare(value, translation())) {
        m_model->setWorldTranslation(Util::toVector3(value));
        emit translationChanged();
    }
}

QQuaternion ModelProxy::orientation() const
{
    Q_ASSERT(m_model);
    return Util::fromQuaternion(m_model->worldOrientation());
}

void ModelProxy::setOrientation(const QQuaternion &value)
{
    Q_ASSERT(m_model);
    if (!qFuzzyCompare(value, orientation())) {
        m_model->setWorldOrientation(Util::toQuaternion(value));
        emit orientationChanged();
    }
}

QVector3D ModelProxy::eulerOrientation() const
{
    Q_ASSERT(m_model);
    Scalar yaw, pitch, roll;
    Matrix3x3 matrix(m_model->worldOrientation());
    matrix.getEulerZYX(yaw, pitch, roll);
    return QVector3D(qRadiansToDegrees(roll), qRadiansToDegrees(pitch), qRadiansToDegrees(yaw));
}

void ModelProxy::setEulerOrientation(const QVector3D &value)
{
    Q_ASSERT(m_model);
    if (!qFuzzyCompare(eulerOrientation(), value)) {
        Quaternion rotation(Quaternion::getIdentity());
        rotation.setEulerZYX(qDegreesToRadians(value.z()), qDegreesToRadians(value.y()), qDegreesToRadians(value.x()));
        m_model->setWorldOrientation(rotation);
        emit orientationChanged();
    }
}

qreal ModelProxy::scaleFactor() const
{
    Q_ASSERT(m_model);
    return static_cast<qreal>(m_model->scaleFactor());
}

void ModelProxy::setScaleFactor(qreal value)
{
    Q_ASSERT(m_model);
    if (!qFuzzyCompare(value, scaleFactor())) {
        m_model->setScaleFactor(static_cast<Scalar>(value));
        emit scaleFactorChanged();
    }
}

qreal ModelProxy::opacity() const
{
    Q_ASSERT(m_model);
    return static_cast<qreal>(m_model->opacity());
}

void ModelProxy::setOpacity(qreal value)
{
    Q_ASSERT(m_model);
    if (!qFuzzyCompare(value, opacity())) {
        m_model->setOpacity(static_cast<Scalar>(value));
        emit opacityChanged();
    }
}

qreal ModelProxy::edgeWidth() const
{
    Q_ASSERT(m_model);
    return static_cast<qreal>(m_model->edgeWidth());
}

void ModelProxy::setEdgeWidth(qreal value)
{
    Q_ASSERT(m_model);
    if (!qFuzzyCompare(value, edgeWidth())) {
        m_model->setEdgeWidth(static_cast<IVertex::EdgeSizePrecision>(value));
        emit edgeWidthChanged();
    }
}

int ModelProxy::maxUVCount() const
{
    Q_ASSERT(m_model);
    return m_model->maxUVCount();
}

void ModelProxy::setMaxUVCount(int value)
{
    Q_ASSERT(m_model);
    if (maxUVCount() != value) {
        m_model->setMaxUVCount(value);
        markDirty();
        emit numUVAChanged();
    }
}

int ModelProxy::orderIndex() const
{
    Q_ASSERT(m_parentProjectRef);
    static const QString kSettingOrderKey(QString::fromStdString(XMLProject::kSettingOrderKey));
    return m_parentProjectRef->modelSetting(this, kSettingOrderKey).toInt();
}

void ModelProxy::setOrderIndex(int value)
{
    Q_ASSERT(m_parentProjectRef);
    if (value != orderIndex()) {
        static const QString kSettingOrderKey(QString::fromStdString(XMLProject::kSettingOrderKey));
        m_parentProjectRef->setModelSetting(this, kSettingOrderKey, value);
        emit orderIndexChanged();
    }
}

bool ModelProxy::isVisible() const
{
    Q_ASSERT(m_model);
    return m_model->isVisible();
}

void ModelProxy::setVisible(bool value)
{
    Q_ASSERT(m_model);
    if (value != isVisible()) {
        m_model->setVisible(value);
        emit visibleChanged();
    }
}

bool ModelProxy::isMoving() const
{
    return m_moving;
}

bool ModelProxy::isDirty() const
{
    return m_dirty;
}

void ModelProxy::setDirty(bool value)
{
    if (isDirty() != value) {
        m_dirty = value;
        emit dirtyChanged();
    }
}

void ModelProxy::markDirty()
{
    setDirty(true);
}

QList<LabelRefObject *> ModelProxy::allLabelRefs() const
{
    return m_allLabels;
}

QList<BoneRefObject *> ModelProxy::allBoneRefs() const
{
    return m_allBones;
}

QList<MorphRefObject *> ModelProxy::allMorphRefs() const
{
    return m_allMorphs;
}

QList<MaterialRefObject *> ModelProxy::allMaterialRefs() const
{
    return m_allMaterials;
}

QList<VertexRefObject *> ModelProxy::allVertexRefs() const
{
    return m_allVertices;
}

QList<RigidBodyRefObject *> ModelProxy::allRigidBodyRefs() const
{
    return m_allRigidBodies;
}

QList<JointRefObject *> ModelProxy::allJointRefs() const
{
    return m_allJoints;
}

QList<IKConstraintRefObject *> ModelProxy::allIKConstraintRefs() const
{
    return m_allIKConstraints;
}

void ModelProxy::resetLanguage()
{
    /* force updating language property */
    m_language = ProjectProxy::DefaultLauguage;
    emit languageChanged();
}

void ModelProxy::initializeAllBones(const Array<ILabel *> &labelRefs)
{
    if (m_allBones.isEmpty()) {
        Array<IBone *> bones;
        m_model->getBoneRefs(bones);
        const int nbones = bones.count();
        for (int i = 0; i < nbones; i++) {
            IBone *boneRef = bones[i];
            const QUuid uuid = QUuid::createUuid();
            BoneRefObject *boneObject = new BoneRefObject(this, boneRef, uuid);
            m_allBones.append(boneObject);
            m_bone2Refs.insert(boneRef, boneObject);
            m_name2BoneRefs.insert(boneObject->name(), boneObject);
            m_uuid2BoneRefs.insert(uuid, boneObject);
        }
        qSort(m_allBones.begin(), m_allBones.end(), Util::LessThan());
        const int nlabels = labelRefs.count();
        for (int i = 0; i < nlabels; i++) {
            ILabel *labelRef = labelRefs[i];
            const int nobjects = labelRef->count();
            if (nobjects > 0 && labelRef->boneRef(0)) {
                LabelRefObject *labelObject = new LabelRefObject(this, labelRef);
                for (int j = 0; j < nobjects; j++) {
                    IBone *boneRef = labelRef->boneRef(j);
                    BoneRefObject *boneObject = m_bone2Refs.value(boneRef);
                    if (boneRef->isInteractive()) {
                        labelObject->addBone(boneObject);
                    }
                }
                m_allLabels.append(labelObject);
            }
        }
    }
}

void ModelProxy::initializeAllMorphs(const Array<ILabel *> &labelRefs, bool all)
{
    if (m_allMorphs.isEmpty()) {
        const IEncoding *encodingRef = m_parentProjectRef->encodingInstanceRef();
        const IString *opacityMorphName = encodingRef->stringConstant(IEncoding::kOpacityMorphAsset);
        Array<IMorph *> morphs;
        m_model->getMorphRefs(morphs);
        const int nmorphs = morphs.count();
        for (int i = 0; i < nmorphs; i++) {
            IMorph *morphRef = morphs[i];
            if (!all && morphRef->category() == IMorph::kBase) {
                continue;
            }
            const QUuid uuid = QUuid::createUuid();
            MorphRefObject *morphObject = new MorphRefObject(this, morphRef, uuid);
            morphObject->initialize();
            m_allMorphs.append(morphObject);
            m_morph2Refs.insert(morphRef, morphObject);
            m_name2MorphRefs.insert(morphObject->name(), morphObject);
            m_uuid2MorphRefs.insert(uuid, morphObject);
            if (m_model->type() == IModel::kAssetModel && morphRef->name(IEncoding::kDefaultLanguage)->equals(opacityMorphName)) {
                morphRef->setWeight(1);
            }
        }
        qSort(m_allMorphs.begin(), m_allMorphs.end(), Util::LessThan());
        const int nlabels = labelRefs.count();
        for (int i = 0; i < nlabels; i++) {
            ILabel *labelRef = labelRefs[i];
            const int nobjects = labelRef->count();
            if (nobjects > 0 && labelRef->morphRef(0)) {
                LabelRefObject *labelObject = new LabelRefObject(this, labelRef);
                for (int j = 0; j < nobjects; j++) {
                    IMorph *morphRef = labelRef->morphRef(j);
                    if (MorphRefObject *morphObject = m_morph2Refs.value(morphRef)) {
                        labelObject->addMorph(morphObject);
                    }
                }
                m_allLabels.append(labelObject);
            }
        }
    }
}

void ModelProxy::initializeAllVertices()
{
    if (m_allVertices.isEmpty()) {
        Array<IVertex *> vertexRefs;
        m_model->getVertexRefs(vertexRefs);
        const int nvertices = vertexRefs.count();
        for (int i = 0; i < nvertices; i++) {
            IVertex *vertexRef = vertexRefs[i];
            VertexRefObject *vertex = new VertexRefObject(this, vertexRef, QUuid::createUuid());
            m_allVertices.append(vertex);
            m_vertex2Refs.insert(vertexRef, vertex);
            m_uuid2VertexRefs.insert(vertex->uuid(), vertex);
        }
        qSort(m_allVertices.begin(), m_allVertices.end(), Util::LessThan());
    }
}

void ModelProxy::initializeAllMaterials()
{
    if (m_allMaterials.isEmpty()) {
        Array<IMaterial *> materialRefs;
        m_model->getMaterialRefs(materialRefs);
        const int nmaterials = materialRefs.count();
        for (int i = 0; i < nmaterials; i++) {
            IMaterial *materialRef = materialRefs[i];
            MaterialRefObject *material = new MaterialRefObject(this, materialRef, QUuid::createUuid());
            connect(material, &MaterialRefObject::texturePathDidChange, this, &ModelProxy::texturePathDidChange);
            m_allMaterials.append(material);
            m_material2Refs.insert(materialRef, material);
            m_name2MaterialRefs.insert(material->name(), material);
            m_uuid2MaterialRefs.insert(material->uuid(), material);
        }
        qSort(m_allMaterials.begin(), m_allMaterials.end(), Util::LessThan());
    }
}

void ModelProxy::initializeAllRigidBodies()
{
    if (m_allRigidBodies.isEmpty()) {
        Array<IRigidBody *> rigidBodyRefs;
        m_model->getRigidBodyRefs(rigidBodyRefs);
        const int nbodies = rigidBodyRefs.count();
        for (int i = 0; i < nbodies; i++) {
            IRigidBody *rigidBodyRef = rigidBodyRefs[i];
            RigidBodyRefObject *rigidBody = new RigidBodyRefObject(this, rigidBodyRef, QUuid::createUuid());
            m_allRigidBodies.append(rigidBody);
            m_rigidBody2Refs.insert(rigidBodyRef, rigidBody);
            m_name2RigidBodyRefs.insert(rigidBody->name(), rigidBody);
            m_uuid2RigidBodyRefs.insert(rigidBody->uuid(), rigidBody);
        }
        qSort(m_allRigidBodies.begin(), m_allRigidBodies.end(), Util::LessThan());
    }
}

void ModelProxy::initializeAllJoints()
{
    if (m_allJoints.isEmpty()) {
        Array<IJoint *> jointRefs;
        m_model->getJointRefs(jointRefs);
        const int njoints = jointRefs.count();
        for (int i = 0; i < njoints; i++) {
            IJoint *jointRef = jointRefs[i];
            JointRefObject *joint = new JointRefObject(this, jointRef, QUuid::createUuid());
            m_allJoints.append(joint);
            m_joint2Refs.insert(jointRef, joint);
            m_name2JointRefs.insert(joint->name(), joint);
            m_uuid2JointRefs.insert(joint->uuid(), joint);
        }
        qSort(m_allJoints.begin(), m_allJoints.end(), Util::LessThan());
    }
}

void ModelProxy::initializeAllIKConstraints()
{
    if (m_allIKConstraints.isEmpty()) {
        Array<IBone::IKConstraint *> constraintRefs;
        m_model->getIKConstraintRefs(constraintRefs);
        const int nconstraints = constraintRefs.count();
        for (int i = 0; i < nconstraints; i++) {
            IBone::IKConstraint *constraintRef = constraintRefs[i];
            IKConstraintRefObject *constraint = new IKConstraintRefObject(this, constraintRef, QUuid::createUuid(), i);
            constraint->initialize();
            m_allIKConstraints.append(constraint);
            m_constraint2Refs.insert(constraintRef, constraint);
            m_uuid2ConstraintRefs.insert(constraint->uuid(), constraint);
        }
        qSort(m_allIKConstraints.begin(), m_allIKConstraints.end(), Util::LessThan());
    }
}

void ModelProxy::saveTransformState()
{
    foreach (BoneRefObject *bone, m_allBones) {
        m_transformState.insert(bone, InternalTransform(bone->rawLocalTranslation(), bone->rawLocalOrientation()));
    }
}

void ModelProxy::clearTransformState()
{
    m_transformState.clear();
}
