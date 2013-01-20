
#include "CPUTCheckboxBase.h"
#include "CPUTStaticOGL31.h"

#include <gl/glew.h>

namespace CPUTGL31
{
#define CPUT_NUM_IMAGES_IN_CHECKBOX_SET 3

class CPUTCheckbox:public CPUTCheckboxBase
{
public:
    CPUTCheckbox(const cString ControlText, CPUTControlID id);
    virtual ~CPUTCheckbox();
    
    void Draw(HDC hdc); 
    
    //CPUTEventHandler
    virtual CPUTEventHandledCode OnKeyboardEvent(CPUTKey key);
    virtual CPUTEventHandledCode CPUTCheckbox::OnMouseEvent(int x, int y, int wheel, CPUTMouseState state);

    //CPUTControl    
    bool ContainsPoint(int x, int y);
    void SetText(const cString String);
    void GetText(char** ppString);
    void GetDimensions(int& width, int& height);
    void SetPosition(int x, int y);

    // Register assets
    static CPUTResult RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID);
    static CPUTResult UnRegisterStaticResources();



private:
    // static varibles used by ALL checkbox controls
    static GLuint m_shaderProgram;
    static GLint m_ModelViewMatrixID;
    static GLint m_textureUniformLocation;
    static GLuint m_pCheckboxTextureList[CPUT_NUM_IMAGES_IN_CHECKBOX_SET];
    static CPUT_SIZE m_pCheckboxTextureSizeList[CPUT_NUM_IMAGES_IN_CHECKBOX_SET];

    // instance variables
    GLuint m_VertexArrayObjectID;
    GLuint m_pVertexBufferObjectIDList[2];     
    CPUTStatic* m_pCheckboxText;

    // helper functions
    CPUTResult RegisterInstanceResources();
    CPUTResult UnRegisterInstanceResources();
    void GetTextPosition(int& x, int& y);   
    void CalculateBounds(int& width, int& height);
    void BuildOrthoProjMat(float *m, float left, float right, float bottom, float top, float znear, float zfar);
    CPUTResult RegisterQuad(int width, int height, GLuint& objectID, GLuint* pVertexBufferObjectIDList);
};
}