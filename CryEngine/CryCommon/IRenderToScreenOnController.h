#ifndef _IRENDERTOSCREENONCONTROLLER_H_
#define _IRENDERTOSCREENONCONTROLLER_H_

#define TEXT_GRID_X 92.0f
#define TEXT_GRID_Y 24.0f

class CRenderToControllerCallbackList;
struct IRenderToScreenOnController;

struct IRenderToControllerCallback
{
	public:
	virtual void RenderToController(IRenderToScreenOnController * renderer) = 0;
	virtual ~IRenderToControllerCallback() { }
};

struct IRenderToScreenOnController
{
	public:
	virtual void Init() = 0;
	virtual void DoRender() = 0;
	virtual void SetClearColour(float r, float g, float b) = 0;
	virtual void RenderQuad(float left, float top, float right, float bottom, float r, float g, float b, float a, int texID = -1) = 0;
	virtual void RenderText(float left, float top, float r, float g, float b, bool bSpacing, char * str) = 0; 
	virtual void TextSetGridSize(float x = TEXT_GRID_X, float y = TEXT_GRID_Y) = 0;
	virtual void AddCallback(IRenderToControllerCallback * callback) = 0;
	virtual void RemoveCallback(IRenderToControllerCallback * callback) = 0;
	virtual ~IRenderToScreenOnController() { }
};

#endif // _IRENDERTOSCREENONCONTROLLER_H_
