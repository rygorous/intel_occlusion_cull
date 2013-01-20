#ifndef __CPUTGUICONTROLLEROGL31_H__ 
#define __CPUTGUICONTROLLEROGL31_H__

#include "CPUTGuiControllerBase.h"
#include "CPUTShaderLoaderOGL31.h"
#include "CPUTStaticOGL31.h"
#include "CPUTButtonOGL31.h"
#include "CPUTCheckboxOGL31.h"
#include "CPUTSliderOGL31.h"
#include "CPUTDropdownOGL31.h"

namespace CPUTGL31
{
    // the GUI controller class that dispatches the rendering calls to all the buttons
    //--------------------------------------------------------------------------------
    class CPUTGuiController:public CPUTGuiControllerBase
    {
    public:
        static CPUTGuiController* GetController();    
        ~CPUTGuiController();

        // initialization
        CPUTResult Initialize();
        CPUTResult GetShaderProgram(GLuint& programID);

        // Control creation/deletion 'helpers'
        CPUTResult CreateButton(const cString pButtonText, CPUTControlID controlID, CPUTControlID panelID, CPUTButton** ppButton);
        CPUTResult CreateSlider(const cString pSliderText, CPUTControlID controlID, CPUTControlID panelID, CPUTSlider** ppSlider);
        CPUTResult CreateCheckbox(const cString pCheckboxText, CPUTControlID controlID, CPUTControlID panelID, CPUTCheckbox** ppChecbox);
        CPUTResult CreateDropdown(const cString pSelectionText, CPUTControlID controlID, CPUTControlID panelID, CPUTDropdown** ppDropdown);
        CPUTResult CreateStatic(const cString Text, CPUTControlID panelID, CPUTControlID controlID=-1, CPUTStatic** ppStatic=NULL);    
        CPUTResult DeleteControl(CPUTControlID controlID);

        // draw routine
        void Draw(HDC hdc);

    private:
        // Static resource 
        CPUTGuiController();
        static CPUTGuiController* m_guiController;

        // Set GUI state for drawing
        void SetGUIDrawingState();
        void ClearGUIDrawingState();

        void BuildOrthoProjMat(float *m, float left, float right, float bottom, float top, float znear, float zfar);

        // register shaders
        CPUTResult RegisterGUIResources(cString VertexShader, cString PixelShader, cString DefaultFontFilename);

        // member variables
        bool m_bInitialized;
        GLuint m_shaderProgram;
        GLint m_ProjectionMatrixID;
        GLint m_ModelViewMatrixID;
        GLint m_FillMode[2];
    };

}

#endif // #ifndef __CPUTGUICONTROLLEROGL31_H__ 
