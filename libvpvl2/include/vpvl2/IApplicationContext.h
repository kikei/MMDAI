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

#pragma once
#ifndef VPVL2_IAPPLICATIONCONTEXT_H_
#define VPVL2_IAPPLICATIONCONTEXT_H_

#include "vpvl2/Common.h"
#include "vpvl2/IEffect.h"

#include <stdarg.h>

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{

class IModel;
class IString;
class ITexture;

namespace gl
{
class FrameBufferObject;
}

class VPVL2_API IApplicationContext
{
public:
    enum ShaderType {
        kEdgeVertexShader,
        kEdgeFragmentShader,
        kModelVertexShader,
        kModelFragmentShader,
        kShadowVertexShader,
        kShadowFragmentShader,
        kZPlotVertexShader,
        kZPlotFragmentShader,
        kEdgeWithSkinningVertexShader,
        kModelWithSkinningVertexShader,
        kShadowWithSkinningVertexShader,
        kZPlotWithSkinningVertexShader,
        kModelEffectTechniques,
        kTransformFeedbackVertexShader,
        kMaxShaderType
    };
    enum KernelType {
        kModelSkinningKernel,
        kMaxKernelType
    };
    enum MousePositionType {
        kMouseCursorPosition,
        kMouseLeftPressPosition,
        kMouseMiddlePressPosition,
        kMouseRightPressPosition,
        kMouseWheelPosition,
        kMaxMousePositionType
    };
    enum MatrixTypeFlags {
        kWorldMatrix        = 0x001,
        kViewMatrix         = 0x002,
        kProjectionMatrix   = 0x004,
        kInverseMatrix      = 0x008,
        kTransposeMatrix    = 0x010,
        kCameraMatrix       = 0x020,
        kLightMatrix        = 0x040,
        kShadowMatrix       = 0x080,
        kMaxMatrixTypeFlags = 0x100
    };
    enum TextureTypeFlags {
        kTexture2D             = 0x1,
        kToonTexture           = 0x2,
        kTexture3D             = 0x4,
        kTextureCube           = 0x8,
        kGenerateTextureMipmap = 0x10,
        kSystemToonTexture     = 0x20,
        kAsyncLoadingTexture   = 0x40,
        kMaxTextureTypeFlags   = 0x80
    };
    struct FunctionResolver {
        enum QueryType {
            kQueryVersion,
            kQueryShaderVersion,
            kQueryCoreProfile,
            kMaxQueryType
        };
        virtual ~FunctionResolver() {}
        virtual bool hasExtension(const char *name) const = 0;
        virtual void *resolveSymbol(const char *name) const = 0;
        virtual int query(QueryType type) const = 0;
    };

    struct SharedTextureParameter {
        SharedTextureParameter(IEffect::Parameter *parameter = 0)
            : textureRef(0),
              parameterRef(parameter)
        {
        }
        ~SharedTextureParameter() {
            textureRef = 0;
            parameterRef = 0;
        }
        ITexture *textureRef;
        IEffect::Parameter *parameterRef;
    };

    virtual ~IApplicationContext() {}

    virtual ITexture *uploadEffectTexture(const IString *name, const IEffect *effectRef) = 0;

    /**
     * モデルのテクスチャをサーバ (GPU) にアップロードします.
     *
     * アップロードしたテクスチャの識別子を texture に格納してください。
     * OpenGL の場合は GLuint の値をセットします。
     * 処理中は例外を投げないように処理を行う必要があります。
     *
     * @param name
     * @param texture
     * @param userData
     * @return bool
     */
    virtual ITexture *uploadModelTexture(const IString *name, int flags, void *userData) = 0;

    /**
     * 取得する型に応じた行列を取得します.
     *
     * flags 変数に呼び出し側が取得したい行列の形式をビットの組み合わせで渡します。
     * 実装側は呼び出し側の要求に従って行列の結果を value に格納する必要があります。
     *
     * @param value
     * @param model
     * @param flags
     */
    virtual void getMatrix(float32 value[16], const IModel *model, int flags) const = 0;

    /**
     * 指定された形式のエフェクトのソースを読み込みます.
     *
     * シェーダのソースの読み込みを行います。失敗した場合は返り値として 0 を渡してください。
     * model の type メソッドを用いて読み込むシェーダの切り替えを行います。
     * path に 0 が渡された場合は IEffect::kStandard のフォールバックパスとなるソースを返す必要があります。
     *
     * 処理中は例外を投げないように処理を行う必要があります。
     *
     * このメソッドは Cg 専用でオフスクリーンに割り当てられたエフェクトの読み込みに利用します。
     *
     * @param type
     * @param path
     * @return IString
     */
    virtual IString *loadShaderSource(ShaderType type, const IString *path) = 0;

    /**
     * 指定された形式の (OpenGL の) シェーダのソースを読み込みます.
     *
     * シェーダのソースの読み込みを行います。失敗した場合は返り値として 0 を渡してください。
     * model の type メソッドを用いて読み込むシェーダの切り替えを行います。
     * 処理中は例外を投げないように処理を行う必要があります。
     *
     * @param type
     * @param model
     * @param userData
     * @return IString
     */
    virtual IString *loadShaderSource(ShaderType type, const IModel *model, void *userData) = 0;

    /**
     * 指定された形式の (OpenCL の) カーネルのソースを読み込みます.
     *
     * カーネルのソースの読み込みを行います。失敗した場合は返り値として 0 を渡してください。
     * 処理中は例外を投げないように処理を行う必要があります。
     *
     * @param type
     * @param userData
     * @return IString
     */
    virtual IString *loadKernelSource(KernelType type, void *userData) = 0;

    /**
     * 指定された文字列を IString に変換します.
     *
     * 処理中は例外を投げないように処理を行う必要があります。
     *
     * @param str
     * @return IString
     */
    virtual IString *toUnicode(const uint8 *str) const = 0;

    /**
     * トゥーン色を取得します.
     *
     * 実装側はトゥーン色の結果を value 変数に格納する必要があります。
     * このメソッドは Cg 専用で、トゥーンテクスチャのみ uploadTexture に代わって呼び出します。
     *
     * @param name
     * @param userData
     * @param value
     * @param context
     */
    virtual void getToonColor(const IString *name, Color &value, void *userData) = 0;

    /**
     * ビューポートの大きさを取得します.
     *
     * 実装側は value 変数に width を x に、height を y に設定して格納する必要があります。
     * このメソッドは Cg 専用です。
     *
     * @param value
     */
    virtual void getViewport(Vector3 &value) const = 0;

    /**
     * マウス座標を取得します.
     *
     * 実装側は type 変数に従ってマウス座標を x と y に、クリックされた時間を z に、
     * クリックされたかの真偽値を w に格納する必要があります。
     * このメソッドは Cg 専用です。
     *
     * @param value
     * @param type
     */
    virtual void getMousePosition(Vector4 &value, MousePositionType type) const = 0;

    /**
     * フレーム位置あるいは起動開始からの秒数を取得します.
     *
     * 実装側は sync が true ならフレーム位置を秒数単位に変換した値を、false なら
     * 起動開始からの秒数を value 変数に格納する必要があります。
     * このメソッドは Cg 専用です。
     *
     * @param value
     * @param sync
     */
    virtual void getTime(float &value, bool sync) const = 0;

    /**
     * 前回からの描画の秒数を取得します.
     *
     * 実装側は sync が true なら前回の描写の経過秒数を、false なら
     * 起動開始からの秒数を value 変数に格納する必要があります。
     * このメソッドは Cg 専用です。
     *
     * @param value
     * @param sync
     */
    virtual void getElapsed(float &value, bool sync) const = 0;

    /**
     * アニメーションテクスチャを更新します.
     *
     * 実装側は引数にしたがってアニメーションテクスチャを更新する必要があります。
     * texture 変数に作成時のテクスチャの識別子が格納されているため、
     * これを各 3D API のテクスチャのオブジェクトにキャストしてください。
     * このメソッドは Cg 専用です。
     *
     * @param offset
     * @param speed
     * @param seek
     * @param texture
     * @param sync
     */
    virtual void uploadAnimatedTexture(float32 offset, float32 speed, float32 seek, void *texture) = 0;

    /**
     * モデル名からモデルのインスタンスを返します.
     *
     * ポインタ参照を返すため、delete で解放してはいけません。
     * このメソッドは Cg 専用です。
     *
     * @brief findModel
     * @param name
     * @return IModel
     * @sa effectOwner
     */
    virtual IModel *findEffectModelRef(const IString *name) const = 0;

    /**
     * エフェクトのインスタンスからモデルのインスタンスを返します.
     *
     * ポインタ参照を返すため、delete で解放してはいけません。
     * このメソッドは Cg 専用です。
     *
     * @brief effectOwner
     * @param name
     * @return IModel
     * @sa findModel
     */
    virtual IModel *findEffectModelRef(const IEffect *effect) const = 0;

    /**
     * エフェクト毎に使われるレンダーターゲットに対するフレームバッファを作成します.
     *
     * このメソッドは Cg 専用です。
     *
     * @brief createFrameBufferObject
     * @param size
     * @return
     */
    virtual gl::FrameBufferObject *createFrameBufferObject() = 0;

    /**
     * CgFX のコンパイラに渡す引数を設定します.
     *
     * 引数には予めヒープに確保した IString のインスタンスを渡す必要があります。
     * 呼び出し側でメモリを解放するため、スタック上のメモリを返してはいけません。
     * 渡すことが可能な引数の一覧は cgc のヘルプを参照してください。
     *
     * このメソッドは Cg 専用です。
     *
     * @brief getEffectCompilerArguments
     * @param arguments
     */
    virtual void getEffectCompilerArguments(Array<IString *> &arguments) const = 0;

    virtual void addSharedTextureParameter(const char *name, const SharedTextureParameter &parameter) = 0;

    virtual bool tryGetSharedTextureParameter(const char *name, SharedTextureParameter &parameter) const = 0;

    virtual FunctionResolver *sharedFunctionResolverInstance() const = 0;
};

} /* namespace VPVL2_VERSION_NS */
using namespace VPVL2_VERSION_NS;

} /* namespace vpvl2 */

#endif
