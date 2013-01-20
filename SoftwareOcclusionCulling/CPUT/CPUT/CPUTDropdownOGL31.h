#ifndef __CPUTDROPDOWNOGL31_H__
#define __CPUTDROPDOWNOGL31_H__

#include "CPUTDropdownBase.h"
#include "CPUTServicesWin.h"
#include "CPUTStaticOGL31.h"

namespace CPUTGL31
{
#define CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY 12
#define CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY 18
#define CPUT_NUM_QUADS_IN_CLOSED_DROPDOWN 9
#define CPUT_DROPDOWN_TEXT_PADDING 2

    const int CLeftTop = 0;
    const int CLeftMid = 1;
    const int CLeftBot = 2;
    const int CMidTop = 3;
    const int CMidMid = 4;
    const int CMidBot = 5;
    const int CRightTop = 6;
    const int CRightMid = 7;
    const int CRightBot = 8;
    const int CButtonUp = 9;
    const int CButtonDown = 10;

    const int CTrayLM = 11;
    const int CTrayLB = 12;
    const int CTrayMM = 13;
    const int CTrayMB = 14;
    const int CTrayRM = 15;
    const int CTrayRB = 16;

    const int CTrayHighlight = 17;

    class CPUTDropdown:public CPUTDropdownBase
    {
    public:
        CPUTDropdown(const cString ControlText, CPUTControlID id);
        ~CPUTDropdown();

        //CPUTEventHandler
        virtual CPUTEventHandledCode OnMouseEvent(int x, int y, CPUTMouseState state);

        //CPUTControl    
        virtual void SetEnable(bool in_bEnabled);
        virtual bool IsEnabled(); 
        virtual bool ContainsPoint(int x, int y);
        virtual void SetText(cString ControlText) {;} // doesn't apply to this control
        virtual void GetText(cString& ControlString) {;} // doesn't apply to this control
        virtual void GetDimensions(int& width, int& height);
        virtual void SetPosition(int x, int y);

        //CPUTDropdown
        void NumberOfSelectableItems(unsigned int& count);
        void GetSelectedItem(unsigned int& index);
        void GetSelectedItem(cString& Item);
        void SetSelectedItem(const unsigned int index);
        CPUTResult AddSelectionItem(const cString Item, bool IsSelected=false);
        void DeleteSelectionItem(const unsigned int index);
        void DeleteSelectionItem(const cString string);

        void Draw(HDC hdc);

        // Register assets
        static CPUTResult RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID);
        static CPUTResult UnRegisterStaticResources();

        CPUTResult RegisterInstanceResources();
        CPUTResult UnRegisterInstanceResources();

    private:
        // dropdown globals
        static GLuint m_ModelViewMatrixID;
        static GLuint m_pActiveTextures[CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY];
        static GLuint m_pDisabledTextures[CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY];
        static CPUT_SIZE m_pImageSizeList[CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY];    
        static CPUT_SIZE m_pDisabledImageSizeList[CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY];  

        // instance data
        CPUT_RECT m_ButtonRect;
        CPUT_RECT m_ReadoutRectInside;
        CPUT_RECT m_ReadoutRectOutside;

        CPUT_RECT m_TrayDimensions;
        unsigned int m_NumberOfSelectableItems;
        int m_SelectableItemBufferSize;
        CPUTStatic* m_pSelectedControlText;
        CPUTStatic** m_ppListOfSelectableItems;

        UINT m_VertexStride; // stride and offsets of the image quads we're drawing on
        UINT m_VertexOffset;
        GLuint m_pQuadVertexBufferVBO[2*CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY];
        GLuint m_pQuadVertexBuffers[CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY];

        // helper functions
        CPUTResult RegisterQuad(int w, int h, float depth, GLuint& objectID, GLuint* VertexBufferObjectIDList );
        void UnRegisterAllQuads();

        void CalculateButtonRect(CPUT_RECT& button);
        void CalculateReadoutRect(CPUT_RECT& inner, CPUT_RECT& outer);
        void CalculateTrayRect(CPUT_RECT& inner, CPUT_RECT& outer);

        void CalculateReadoutTextPosition(int& x, int& y);
        void CalculateMaxItemSize(int& width, int& height);
    };
}

#endif //#ifndef __CPUTDROPDOWNOGL31_H__