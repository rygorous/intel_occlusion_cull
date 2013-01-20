#ifndef __CPUTSLIDEROGL31_H__
#define __CPUTSLIDEROGL31_H__

#include "CPUTSliderBase.h"
#include "CPUTStaticOGL31.h"

#include <gl/glew.h>

namespace CPUTGL31
{
#define CPUT_NUM_IMAGES_IN_SLIDER_ARRAY 6
#define CPUT_NUM_QUADS_IN_SLIDER_ARRAY 6
#define CPUT_DEFAULT_TRAY_WIDTH 200

    // Button base - common functionality for all the controls
    //-----------------------------------------------------------------------------
    class CPUTSlider:public CPUTSliderBase
    {
    public:
        // CPUTSlider
        CPUTSlider(const cString ControlText, CPUTControlID id);
        CPUTSlider(CPUTSliderBase& copy);
        virtual ~CPUTSlider();
        void Draw(HDC hdc);

        //CPUTEventHandler
        virtual CPUTEventHandledCode OnKeyboardEvent(CPUTKey key){return CPUT_EVENT_UNHANDLED;} 
        virtual CPUTEventHandledCode OnMouseEvent(int x, int y, CPUTMouseState state);

        // CPUTControl & graphical manipulation
        void SetEnable(bool in_bEnabled) {;} // IMPL!
        bool IsEnabled() {return false;}// IMPL!
        void GetPosition(int& x, int& y);
        void GetDimensions(int& width, int& height);
        void SetPosition(int x, int y); 
        virtual bool ContainsPoint(int x, int y);

        void SetString(cString ControlText);

        //GUI management
        CPUTResult GetDimentions(CPUT_RECT& Rect);

        //CPUTSliderBase    
        CPUTResult SetScale(float StartValue, float EndValue, int NumberOfSteps);
        CPUTResult GetValue(float& fValue);
        CPUTResult SetValue(float fValue);

        // Register assets
        static CPUTResult RegisterStaticResources(GLuint shaderID, GLuint modelViewMatrixID);
        static CPUTResult UnRegisterStaticResources();

        CPUTResult RegisterInstanceResources();
        CPUTResult UnRegisterInstanceResources();


    private:
        struct LocationGuides
        {
            float TickIndent;
            float TextDownIndent;
            float GripDownIndent;
            float TrayDownIndent; 
            float TickSpacing;
            float TotalWidth;
            float TotalHeight;
        };

        CPUTStatic* m_pControlText;
        int m_SliderNubTickLocation;
        float m_SliderNubLocation;

        // instance vertex/image handles
        GLuint m_pQuadVertexBuffers[CPUT_NUM_QUADS_IN_SLIDER_ARRAY];
        GLuint m_pQuadVertexBufferObjectIDList[(2*(CPUT_NUM_QUADS_IN_SLIDER_ARRAY))];    

        // slider global resources
        static GLint m_ModelViewMatrixID;
        static GLuint m_pTextureResViews[CPUT_NUM_IMAGES_IN_SLIDER_ARRAY];
        static GLuint m_pDisabledTextureResViews[CPUT_NUM_IMAGES_IN_SLIDER_ARRAY];
        static CPUT_SIZE m_pImageSizeList[CPUT_NUM_IMAGES_IN_SLIDER_ARRAY];    
        static CPUT_SIZE m_pDisabledImageSizeList[CPUT_NUM_IMAGES_IN_SLIDER_ARRAY];    

        // stride and offsets of the image quads we're drawing on
        UINT m_VertexStride; 
        UINT m_VertexOffset;

        // helper functions
        bool PointingAtNub(int x, int y);
        void CalculateLocationGuides(LocationGuides& guides);
        void SnapToNearestTick();
        CPUTResult RegisterQuad(int width, int height, float depth, GLuint& objectID, GLuint* pVertexBufferObjectIDList);    
    };
}
#endif // #ifndef __CPUTSLIDEROGL31_H__