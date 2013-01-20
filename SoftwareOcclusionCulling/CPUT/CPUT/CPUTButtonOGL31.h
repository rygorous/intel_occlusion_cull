#ifndef __CPUTCONTROLSOGL31_H__ 
#define __CPUTCONTROLSOGL31_H__

#include "CPUTButtonBase.h"

//#include "CPUTGuiControllerOGL31.h"
#include "CPUTStaticOGL31.h"


#include "CPUTTextureLoaderOGL31.h"
#include "CPUTShaderLoaderOGL31.h"


#include <math.h> // for tan()


// this file contains the OGL31-specific rendering portions of each of the controls
//--------------------------------------------------------------------------------
namespace CPUTGL31
{

#define CPUT_NUM_IMAGES_IN_BUTTON 9


    // OGL31 button
    //--------------------------------------------------------------------------------
    class CPUTButton:public CPUTButtonBase
    {
    public:    
        CPUTButton(const cString ControlText, CPUTControlID id);
        virtual ~CPUTButton();

        //Management
        CPUTResult LoadButtonImages(char* in_pButtonFilename);

        //CPUTEventHandler
        virtual CPUTEventHandledCode OnKeyboardEvent(CPUTKey key);
        virtual CPUTEventHandledCode OnMouseEvent(int x, int y, int wheel, CPUTMouseState state);

        //CPUTControl    
        bool ContainsPoint(int x, int y);
        void SetText(const cString String);
        void GetText(char** ppString);
        virtual void SetPosition(int x, int y);
        void GetLocation(int& x, int& y);

        // Register assets
        static CPUTResult RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID);
        static CPUTResult UnRegisterStaticResources();

        void Draw(HDC hdc);

    private:    
        void BuildOrthoProjMat(float *m, float left, float right, float bottom, float top, float znear, float zfar);


        CPUTResult RegisterInstanceResources();
        CPUTResult UnRegisterInstanceResources();
        CPUTResult RegisterQuad(int width, int height, GLuint& objectID, GLuint* VertexBufferObjectIDList);

        CPUTResult Resize(int width, int height);
        void GetInsetTextCoordinate(int &x, int &y);

        // geometry/shaders    
        GLuint m_pIdleVertexArrayObjectIDs[CPUT_NUM_IMAGES_IN_BUTTON];
        GLuint m_pPressedVertexArrayObjectIDs[CPUT_NUM_IMAGES_IN_BUTTON]; 
        GLuint m_pIdleVertexBufferObjectIDs[CPUT_NUM_IMAGES_IN_BUTTON*2]; // 2 VBO's for every VAO    
        GLuint m_pPressedVertexBufferObjectIDs[CPUT_NUM_IMAGES_IN_BUTTON*2]; // 2 VBO's for every VAO
        //GLint m_ProjectionMatrixID;

        // state
        bool m_bMouseInside;
        bool m_bButtonDown;

        // textures
        static bool m_Registered;
        static GLuint m_shaderProgram;
        static GLint m_ModelViewMatrixID;
        static GLint m_textureUniformLocation;
        static GLuint m_pButtonIdleTextureList[CPUT_NUM_IMAGES_IN_BUTTON];
        static CPUT_SIZE m_pButtonIdleImageSizeList[CPUT_NUM_IMAGES_IN_BUTTON];
        static GLuint m_pButtonPressedTextureList[CPUT_NUM_IMAGES_IN_BUTTON];
        static CPUT_SIZE m_pButtonPressedImageSizeList[CPUT_NUM_IMAGES_IN_BUTTON];

        CPUTStatic* m_pButtonText;
        void GetInsideTextCoordinate(int &x, int &y);

        CPUT_SIZE m_pButtonIdleSizeList[CPUT_NUM_IMAGES_IN_BUTTON];
        CPUT_SIZE m_pButtonPressedSizeList[CPUT_NUM_IMAGES_IN_BUTTON];

        // sizes
        static int m_SmallestLeftSizeIdle;
        static int m_SmallestRightSizeIdle;
        static int m_SmallestTopSizeIdle;
        static int m_SmallestBottomSizeIdle;

        static int m_SmallestLeftSizePressed;
        static int m_SmallestRightSizePressed;
        static int m_SmallestTopSizePressed;
        static int m_SmallestBottomSizePressed;

    };
}

#endif // #ifndef __CPUTCONTROLSOGL31_H__ 
