// MODVIEW32View.cpp : implementation of the CMODVIEW32View class
//

#include "stdafx.h"
#include "math.h"
#include "MODVIEW32.h"
#include "modview32doc.h"
#include "MODVIEW32View.h"
#include "DataSafe.h"
#include "Matrix.h"
#include "MainFrm.h"
#include "DM_Reg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#define glRGB(x, y, z)	glColor3ub((GLubyte)x, (GLubyte)y, (GLubyte)z)


/////////////////////////////////////////////////////////////////////////////
// CMODVIEW32View

IMPLEMENT_DYNCREATE(CMODVIEW32View, CView)

BEGIN_MESSAGE_MAP(CMODVIEW32View, CView)
	//{{AFX_MSG_MAP(CMODVIEW32View)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMODVIEW32View construction/destruction

CMODVIEW32View::CMODVIEW32View()
{
	PI=3.141562f;
	m_PosIncr=100.2f;
	m_AngIncr=10.0f;
	m_DirZ=0.0f;
	m_DirY=0.0f;
	m_DirX=0.0f;
	
	m_PosZ=4.0f;
	m_PosY=0.0f;
	m_PosX=0.0f;
	m_AngleX=100.0f;
	m_AngleY=0.0f;
	m_AngleZ=0.0f;
	m_zTranslation=0;
	m_hDC=NULL;
	m_hGLContext=NULL;
	m_GLPixelIndex=0;
	m_Detaillevel=0;
	m_MouseMoveMode=MOUSEMOVEMODE_NONE;
	m_FS_DisplaySubsystem=-1;
	m_QuickRenderMode=0;
	m_Rotation=0;
	m_DoRotation=FALSE;

	m_DisplayTexture=-1;
	m_DisplaySubmodel=-1;
	m_FS_DisplayThruster=-1;
	m_EditorFS_HighLight_Segment_1=-1;
	m_EditorFS_HighLight_Segment_2=-1;
	m_EditorFS_ShowModelInfo=FALSE;
	m_RenderSmooth=TRUE;
	m_RenderCull=FALSE;
	m_RenderZbuffer=TRUE;

	//Render mode
	m_RenderMode=DMReg_ReadHKCUint("RenderMode",RENDER_TEXTURED);
	m_ShowGuns=DMReg_ReadHKCUint("ShowGuns",FALSE);
	m_ShowSegments=DMReg_ReadHKCUint("ShowSegments",FALSE);
	m_ShowShield=DMReg_ReadHKCUint("ShowShield",FALSE);
	m_FS_ShowDockingPoints=DMReg_ReadHKCUint("ShowDockingPoints",FALSE);
	m_FS_ShowThrusterGlows=DMReg_ReadHKCUint("ShowThrusterGlows",FALSE);
	m_ShowThruster=DMReg_ReadHKCUint("ShowThruster",TRUE);
	m_FastTextureLoad=DMReg_ReadHKCUint("FastTextureLoad",TRUE);
	m_RenderZbuffer=DMReg_ReadHKCUint("AntiAlias",TRUE);
	m_ClearColorRed  =(DMReg_ReadHKCUint("BackgroundRed",0))  *1.0f;
	m_ClearColorGreen=(DMReg_ReadHKCUint("BackgroundGreen",0))*1.0f;
	m_ClearColorBlue =(DMReg_ReadHKCUint("BackgroundBlue",0)) *1.0f;
	m_SkipBuild=FALSE;
}

CMODVIEW32View::~CMODVIEW32View()
{
}

BOOL CMODVIEW32View::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= (WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	cs.lpszClass=AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC,
		::LoadCursor(NULL, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1), NULL);

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMODVIEW32View drawing

void CMODVIEW32View::OnDraw(CDC* pDC)
{
	CMODVIEW32Doc* pDoc=GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CMODVIEW32View diagnostics

#ifdef _DEBUG
void CMODVIEW32View::AssertValid() const
{
	CView::AssertValid();
}

void CMODVIEW32View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMODVIEW32Doc* CMODVIEW32View::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMODVIEW32Doc)));
	return (CMODVIEW32Doc*)m_pDocument;
}
#endif //_DEBUG

CMainFrame *CMODVIEW32View::GetMainFrame()
{
	CMainFrame *viewFrame=static_cast<CMainFrame*>(GetParentFrame());
	return viewFrame;
}

/////////////////////////////////////////////////////////////////////////////
// CMODVIEW32View message handlers

int CMODVIEW32View::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct)==-1)
		return -1;
	
	SetTimer(3,150,NULL);

	HWND hWnd=GetSafeHwnd();
	m_hDC=::GetDC(hWnd);
	
	if(!SetWindowPixelFormat(m_hDC))
		return 0;
	if(!CreateViewGLContext(m_hDC))
		return 0;
	
	// Default mode
	GLfloat  ambientLight[]={ 0.3f, 0.3f, 0.3f, 1.0f};
	GLfloat  diffuseLight[]={ 0.7f, 0.7f, 0.7f, 1.0f};
	GLfloat  specular[]={ 1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat lightPos[]={ 0.0f, 350.0f, 350.0f, 1.0f};
	GLfloat  specref[]= { 1.0f, 1.0f, 1.0f, 1.0f};

	glEnable(GL_DEPTH_TEST);	// Hidden surface removal
	glFrontFace(GL_CW);		// Counter clock-wise polygons face out

	// Enable lighting
	glEnable(GL_LIGHTING);

	// Setup and enable light 0
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
	glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
	glEnable(GL_LIGHT0);

	// Enable color tracking
	glEnable(GL_COLOR_MATERIAL);

	// Set Material properties to follow glColor values
	//glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// All materials hereafter have full specular reflectivity
	// with a high shine
	//glMaterialfv(GL_FRONT, GL_SPECULAR, specref);
	//glMateriali(GL_FRONT,GL_SHININESS,128);

	//Set background color
	glClearColor(m_ClearColorRed,m_ClearColorGreen,m_ClearColorBlue,1.0f);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_S);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);

	return 0;
}

void CMODVIEW32View::OnDestroy() 
{
	if(wglGetCurrentContext()!=NULL)
		wglMakeCurrent(NULL,NULL);
	
	if(m_hGLContext!=NULL)
	{
		wglDeleteContext(m_hGLContext);
		m_hGLContext=NULL;
	}

	CView::OnDestroy();
}

BOOL CMODVIEW32View::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;  
}

void CMODVIEW32View::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	//Set OpenGL perspective, viewport and mode
	CSize size(cx,cy);
	m_ClientRect.right=cx;
	m_ClientRect.bottom=cy;
	BOOL res=wglMakeCurrent(m_hDC,m_hGLContext);
	ASSERT(res);
	
	GLsizei w=size.cx;
	GLsizei h=size.cy;
	ChangeSize(w, h);
}

void CMODVIEW32View::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CMODVIEW32Doc *pDoc=(CMODVIEW32Doc *)GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->test3="qwertzuiop";
	pDoc->test4="qwertzuiop";

	if(pDoc->m_ModelJustLoaded)
	{
		m_DisplayTexture=-1;
	}
	pDoc->m_ModelJustLoaded=FALSE;
	
	if(pDoc->m_DoResetGeometry)
		ResetGeometry();
	pDoc->m_DoResetGeometry=FALSE;
	
	wglMakeCurrent(m_hDC,m_hGLContext);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(m_ClearColorRed,m_ClearColorGreen,m_ClearColorBlue,1.0f);
	
	//RenderIndicator(); //HeikoH @ Build 16: this is the cause of the bad memory leak so I just disabled it
	glPushMatrix();
	
	//Position / translation / scale
	//OLD
	glLoadIdentity();
		glTranslated(m_xTranslation,m_yTranslation,m_zTranslation);
		glRotatef(m_xRotation,1.0,0.0,0.0);
		glRotatef(m_yRotation,0.0,1.0,0.0);
		glScalef(m_xScaling,m_yScaling,m_zScaling);
		//glTranslatef(0,0,-GetDocument()->m_RF_Model.radius*3);
	//NEW
		//gluLookAt(m_PosX,m_PosY,m_PosZ,m_DirX,m_DirY,m_DirZ,0.0f,1.0f,0.0f);
	
	//Render
	if(pDoc->m_ModelLoaded)
	{
		if (pDoc->m_Game == GAME_FS)
		{
			if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED && pDoc->m_NoTexturesFound)
			{
				GetMainFrame()->SetTypeOfView(TOV_MODELVIEWBUTNOTEXTURES);
				//break;
				//PaintErrorMessage("None of the textures could be found! Switch to wireframe or surface mode.");
				//\r\nYou should switch to wireframe or surfaced render mode\to see anything at all.\nAlso check File|Options for the reason of the missing textures.");
			}
			else
			{
				if(!SmartRenderEngine_SkipBuild())
					FS_BuildScene();
				glCallList(pDoc->m_Model);
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				SwapBuffers(m_hDC);
			}
		} else ASSERT(FALSE);
	}
	else
	{
		CString err;
		TRACE("xxx=%i\n",GetDocument()->m_ErrorCode);
		switch(GetDocument()->m_ErrorCode)
		{
		case ERROR_GEN_NOERROR:
			GetMainFrame()->SetTypeOfView(TOV_NODOCUMENT);
			err="No model loaded.";
			break;

		case ERROR_GEN_NOMODELSELECTED:
			GetMainFrame()->SetTypeOfView(TOV_ARCHIVELOADED);
			err="No model selected.";
			break;

		default:
			GetMainFrame()->SetTypeOfView(TOV_MODELVIEWBUTFAILED);
			err.Format("Error %i: %s",GetDocument()->m_ErrorCode,GetDocument()->m_ErrorString);
		}

		PaintErrorMessage(err);
	}
	GetMainFrame()->UpdateNavStatus();
	m_SkipBuild=FALSE;
}

void CMODVIEW32View::PaintErrorMessage(CString x)
{
	char cBuffer[256];
	RECT cRect;
	strcpy_s(cBuffer,x);

	//CFont fon;
	//fon.CreateFont(-11,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,0,FALSE,FALSE,FALSE,DEFAULT_PITCH | FF_SWISS | TMPF_TRUETYPE,"Tahoma");
	//CFont *old_fon=dc.SelectObject(&fon);

	SwapBuffers(m_hDC);

	GetClientRect(&cRect);
	SetBkColor(m_hDC,RGB(0,0,0));
	SetTextColor(m_hDC,RGB(255,255,255));
	TextOut(m_hDC,0,0,cBuffer,strlen(cBuffer));
	//dc.SelectObject(old_fon);
}

//********************************************
// SetWindowPixelFormat
//********************************************
BOOL CMODVIEW32View::SetWindowPixelFormat(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pixelDesc;
	
	pixelDesc.nSize=sizeof(PIXELFORMATDESCRIPTOR);
	pixelDesc.nVersion=1;
	
	pixelDesc.dwFlags=PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER | PFD_STEREO_DONTCARE;
	
	pixelDesc.iPixelType=PFD_TYPE_RGBA;
	pixelDesc.cColorBits=32;
	pixelDesc.cRedBits=8;
	pixelDesc.cRedShift=16;
	pixelDesc.cGreenBits=8;
	pixelDesc.cGreenShift=8;
	pixelDesc.cBlueBits=8;
	pixelDesc.cBlueShift=0;
	pixelDesc.cAlphaBits=0;
	pixelDesc.cAlphaShift=0;
	pixelDesc.cAccumBits=64;
	pixelDesc.cAccumRedBits=16;
	pixelDesc.cAccumGreenBits=16;
	pixelDesc.cAccumBlueBits=16;
	pixelDesc.cAccumAlphaBits=0;
	pixelDesc.cDepthBits=32;
	pixelDesc.cStencilBits=8;
	pixelDesc.cAuxBuffers=0;
	pixelDesc.iLayerType=PFD_MAIN_PLANE;
	pixelDesc.bReserved=0;
	pixelDesc.dwLayerMask=0;
	pixelDesc.dwVisibleMask=0;
	pixelDesc.dwDamageMask=0;
	
	m_GLPixelIndex=ChoosePixelFormat(hDC,&pixelDesc);
	if(m_GLPixelIndex==0) // Choose default
	{
		m_GLPixelIndex=1;
		if(DescribePixelFormat(m_hDC,m_GLPixelIndex,
			sizeof(PIXELFORMATDESCRIPTOR),&pixelDesc)==0)
			return FALSE;
	}
	
	return SetPixelFormat(m_hDC,m_GLPixelIndex,&pixelDesc);
}

//********************************************
// CreateViewGLContext
// Create an OpenGL rendering context
//********************************************
BOOL CMODVIEW32View::CreateViewGLContext(HDC hDC)
{
	m_hGLContext=wglCreateContext(hDC);
	
	if(m_hGLContext==NULL)
		return FALSE;
	
	return wglMakeCurrent(m_hDC,m_hGLContext);
}

//********************************************
// ResetGeometry
//********************************************
void CMODVIEW32View::ResetGeometry(void)
{
	CMODVIEW32Doc *pDoc=(CMODVIEW32Doc *)GetDocument();
	m_xRotation=30.0f;
	m_yRotation=30.0f;
	m_xScaling=1.0f;
	m_yScaling=1.0f;
	m_zScaling=1.0f;
	m_xTranslation=0;
	m_yTranslation=0;
	m_zTranslation=0;
	//if(GetDocument()->m_Game==GAME_RF)
	//	m_zTranslation=-GetDocument()->m_RF_Model.radius*3;
}

HPALETTE CMODVIEW32View::GetOpenGLPalette(HDC hDC)
{
	HPALETTE hRetPal=NULL;		// Handle to palette to be created
	PIXELFORMATDESCRIPTOR pfd;	// Pixel Format Descriptor
	LOGPALETTE *pPal;			// Pointer to memory for logical palette
	int nPixelFormat;			// Pixel format index
	int nColors;				// Number of entries in palette
	int i;						// Counting variable
	BYTE RedRange,GreenRange,BlueRange;
								// Range for each color entry (7,7,and 3)


	// Get the pixel format index and retrieve the pixel format description
	nPixelFormat=GetPixelFormat(hDC);
	DescribePixelFormat(hDC, nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	// Does this pixel format require a palette?  If not, do not create a
	// palette and just return NULL
	if(!(pfd.dwFlags & PFD_NEED_PALETTE))
		return NULL;

	// Number of entries in palette.  8 bits yeilds 256 entries
	nColors=1<<pfd.cColorBits;

	// Allocate space for a logical palette structure plus all the palette entries
	pPal=(LOGPALETTE*)malloc(sizeof(LOGPALETTE) +nColors*sizeof(PALETTEENTRY));

	if (pPal) {
		// Fill in palette header
		pPal->palVersion=0x300;		// Windows 3.0
		pPal->palNumEntries=nColors; // table size

		// Build mask of all 1's.  This creates a number represented by having	
		// the low order x bits set, where x=pfd.cRedBits, pfd.cGreenBits, and
		// pfd.cBlueBits.
		RedRange  =(1<<pfd.cRedBits)-1;
		GreenRange=(1<<pfd.cGreenBits)-1;
		BlueRange =(1<<pfd.cBlueBits)-1;

		// Loop through all the palette entries
		for(i=0; i<nColors; i++)
		{
			// Fill in the 8-bit equivalents for each component
			pPal->palPalEntry[i].peRed=(i>>pfd.cRedShift) & RedRange;
			pPal->palPalEntry[i].peRed=(unsigned char)(
				(double) pPal->palPalEntry[i].peRed * 255.0 / RedRange);

			pPal->palPalEntry[i].peGreen=(i>>pfd.cGreenShift) & GreenRange;
			pPal->palPalEntry[i].peGreen=(unsigned char)(
				(double)pPal->palPalEntry[i].peGreen * 255.0 / GreenRange);

			pPal->palPalEntry[i].peBlue=(i>>pfd.cBlueShift) & BlueRange;
			pPal->palPalEntry[i].peBlue=(unsigned char)(
				(double)pPal->palPalEntry[i].peBlue * 255.0 / BlueRange);

			pPal->palPalEntry[i].peFlags=(unsigned char) NULL;
		}

			// Create the palette
			hRetPal=CreatePalette(pPal);

			// Go ahead and select and realize the palette for this device context
			SelectPalette(hDC,hRetPal,FALSE);
			RealizePalette(hDC);
	}

	// Free the memory used for the logical palette structure
	free(pPal);

	// Return the handle to the new palette
	return hRetPal;
}


void CMODVIEW32View::FS_SetDetailLevel(int mode)
{
	CMODVIEW32Doc* pDoc=GetDocument();
	m_Detaillevel=mode;
	if(m_RenderMode==RENDER_TEXTURED)
		pDoc->FS_LoadTextureData(mode,m_ShowThruster,m_FastTextureLoad);
	CMainFrame *viewFrame=static_cast<CMainFrame*>(GetParentFrame());
	viewFrame->FS_SetDetailLevel(mode);
	InvalidateRect(NULL);
}

void CMODVIEW32View::FS_BuildScene(void)
{
	BOOL ValidTexture;
	float v[35][3];
	float n[35][3];
	float uv[35][2];
	float normal[3];
	BOOL data2type, data3type;

	data2type=FALSE;
	data3type=FALSE;

	glClearColor(m_ClearColorRed,m_ClearColorGreen,m_ClearColorBlue,1.0f);

	CMODVIEW32Doc* pDoc=GetDocument();
	ASSERT(pDoc->m_ModelLoaded);

	glNewList(pDoc->m_Model=glGenLists(1), GL_COMPILE);
	glScalef(-1.0,1.0,1.0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());

	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	glShadeModel(GL_SMOOTH);

	//Subsystems stuff
	ASSERT(pDoc->m_FS_NumSOBJ<MAX_FS_SOBJ);
	if(m_FS_DisplaySubsystem!=-1)
	{
		if(m_FS_DisplaySubsystem!=-2)
		{
			ASSERT(m_FS_DisplaySubsystem<MAX_FS_SUBSYSTEMS);
			ASSERT(m_FS_DisplaySubsystem>=0);

			pDoc->m_EditorFS_Subsystem=pDoc->m_FS_Subsystems[m_FS_DisplaySubsystem];
		}
		glDisable(GL_TEXTURE_2D);
		glRGB(FS_GUN_CROSSCOLOR_R,FS_GUN_CROSSCOLOR_G,FS_GUN_CROSSCOLOR_B);
		vms_vector pnt=pDoc->m_EditorFS_Subsystem.Center;
		float rad=pDoc->m_EditorFS_Subsystem.Radius;

		float DIV=0.5;
		glBegin(GL_LINES);
		glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z-rad); glVertex3f(pnt.x-rad*DIV,pnt.y-rad,pnt.z-rad);
		glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z-rad); glVertex3f(pnt.x-rad,pnt.y-rad*DIV,pnt.z-rad);
		glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z-rad); glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z-rad*DIV);
		glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z+rad); glVertex3f(pnt.x-rad*DIV,pnt.y-rad,pnt.z+rad);
		glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z+rad); glVertex3f(pnt.x-rad,pnt.y-rad*DIV,pnt.z+rad);
		glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z+rad); glVertex3f(pnt.x-rad,pnt.y-rad,pnt.z+rad*DIV);
		glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z-rad); glVertex3f(pnt.x-rad*DIV,pnt.y+rad,pnt.z-rad);
		glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z-rad); glVertex3f(pnt.x-rad,pnt.y+rad*DIV,pnt.z-rad);
		glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z-rad); glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z-rad*DIV);
		glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z+rad); glVertex3f(pnt.x-rad*DIV,pnt.y+rad,pnt.z+rad);
		glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z+rad); glVertex3f(pnt.x-rad,pnt.y+rad*DIV,pnt.z+rad);
		glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z+rad); glVertex3f(pnt.x-rad,pnt.y+rad,pnt.z+rad*DIV);
		glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z-rad); glVertex3f(pnt.x+rad*DIV,pnt.y-rad,pnt.z-rad);
		glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z-rad); glVertex3f(pnt.x+rad,pnt.y-rad*DIV,pnt.z-rad);
		glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z-rad); glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z-rad*DIV);
		glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z+rad); glVertex3f(pnt.x+rad*DIV,pnt.y-rad,pnt.z+rad);
		glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z+rad); glVertex3f(pnt.x+rad,pnt.y-rad*DIV,pnt.z+rad);
		glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z+rad); glVertex3f(pnt.x+rad,pnt.y-rad,pnt.z+rad*DIV);
		glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z-rad); glVertex3f(pnt.x+rad*DIV,pnt.y+rad,pnt.z-rad);
		glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z-rad); glVertex3f(pnt.x+rad,pnt.y+rad*DIV,pnt.z-rad);
		glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z-rad); glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z-rad*DIV);
		glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z+rad); glVertex3f(pnt.x+rad*DIV,pnt.y+rad,pnt.z+rad);
		glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z+rad); glVertex3f(pnt.x+rad,pnt.y+rad*DIV,pnt.z+rad);
		glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z+rad); glVertex3f(pnt.x+rad,pnt.y+rad,pnt.z+rad*DIV);
		glEnd();
	}

	//Gun stuff
#define EDITORFS_ALLGUNS_NORMLENGTH 100
#define EDITORFS_TURRETS_NORMLENGTH 100
#define EDITORFS_REALGUNS_NORMLENGTH 10
	ASSERT(pDoc->m_Guns.Num<MAX_GUNS);
	if(m_ShowGuns || m_DisplayGun>=0)
	{
		glDisable(GL_TEXTURE_2D);
		glRGB(FS_GUN_CROSSCOLOR_R,FS_GUN_CROSSCOLOR_G,FS_GUN_CROSSCOLOR_B);
	
		FS_VPNT gunpnt;
		
		int mult;
		for(int j=0;j<pDoc->m_Guns.Num;j++)
		{
			gunpnt=pDoc->m_Guns.Gun[j];
			
			if(pDoc->m_Guns.InSubModel[j])
			{
				gunpnt.x+=pDoc->m_FS_SOBJ[pDoc->m_Guns.InSubModel[j]].offset.x;
				gunpnt.y+=pDoc->m_FS_SOBJ[pDoc->m_Guns.InSubModel[j]].offset.y;
				gunpnt.z+=pDoc->m_FS_SOBJ[pDoc->m_Guns.InSubModel[j]].offset.z;
			}

			if((m_DisplaySubmodel==-1) || (m_DisplaySubmodel==pDoc->m_Guns.InSubModel[j]))
			{
				if((m_DisplayGun==-1) || (pDoc->m_GunsGroup[m_DisplayGun].GunType==pDoc->m_Guns.GunType[j] && pDoc->m_GunsGroup[m_DisplayGun].GunBank==pDoc->m_Guns.GunBank[j]))
				{
					glBegin(GL_LINES);
					DrawCross(gunpnt,FS_GUN_CROSSLENGTH);
					glEnd();

					mult=EDITORFS_REALGUNS_NORMLENGTH;
					if(pDoc->m_Guns.GunType[j]==GUNTYPE_TURRET_GUN || pDoc->m_Guns.GunType[j]==GUNTYPE_TURRET_MIS)
						mult=EDITORFS_TURRETS_NORMLENGTH;
					glPushMatrix();
					FS_RotateParts(pDoc->m_Guns.InSubModel[j]);
					glDisable(GL_TEXTURE_2D);
					glRGB(255,255,255);
					glBegin(GL_LINES);
					glVertex3f(gunpnt.x,gunpnt.y,gunpnt.z);
					glVertex3f(gunpnt.x+pDoc->m_Guns.GunDirection[j].x*mult,gunpnt.y+pDoc->m_Guns.GunDirection[j].y*mult,gunpnt.z+pDoc->m_Guns.GunDirection[j].z*mult);
					glEnd();
					glPopMatrix();
				}
			}
		}
	}
	if(m_DisplayGun==-3) //TURRET
	{
		glDisable(GL_TEXTURE_2D);
	
		for(int i=0;i<GetDocument()->m_EditorFS_Turret.num_firing_points;i++)
		{
			FS_VPNT gunpnt=GetDocument()->m_EditorFS_Turret.firing_point[i];
		
			if(GetDocument()->m_EditorFS_Turret.sobj_parent)
			{
				gunpnt.x+=pDoc->m_FS_SOBJ[GetDocument()->m_EditorFS_Turret.sobj_parent].offset.x;
				gunpnt.y+=pDoc->m_FS_SOBJ[GetDocument()->m_EditorFS_Turret.sobj_parent].offset.y;
				gunpnt.z+=pDoc->m_FS_SOBJ[GetDocument()->m_EditorFS_Turret.sobj_parent].offset.z;
			}

			int mult=1;
			if(m_EditorFS_TGUN_FiringPoint!=i)
				glRGB(FS_GUN_CROSSCOLOR_R,FS_GUN_CROSSCOLOR_G,FS_GUN_CROSSCOLOR_B);
			else
			{
				glRGB(255,255,255);
				mult=4;
			}

			glPushMatrix();
			FS_RotateParts(GetDocument()->m_EditorFS_Turret.sobj_parent);
			glBegin(GL_LINES);
			DrawCross(gunpnt,mult*FS_GUN_CROSSLENGTH);
			/*glEnd();

			glRGB(255,255,0);
			glBegin(GL_LINES);*/
			glVertex3f(gunpnt.x,gunpnt.y,gunpnt.z);
			glVertex3f(gunpnt.x+GetDocument()->m_EditorFS_Turret.turret_normal.x*EDITORFS_TURRETS_NORMLENGTH,gunpnt.y+GetDocument()->m_EditorFS_Turret.turret_normal.y*EDITORFS_TURRETS_NORMLENGTH,gunpnt.z+GetDocument()->m_EditorFS_Turret.turret_normal.z*EDITORFS_TURRETS_NORMLENGTH);
			glEnd();
			glPopMatrix();
		}
	}

	if(m_DisplayGun==-4) //REALGUN
	{
		glDisable(GL_TEXTURE_2D);
	
		for(int i=0;i<GetDocument()->m_EditorFS_RealGun.num_guns;i++)
		{
			FS_VPNT gunpnt=GetDocument()->m_EditorFS_RealGun.point[i];
			FS_VPNT gunnrm=GetDocument()->m_EditorFS_RealGun.normal[i];
		
			int mult=1;
			if(m_EditorFS_GPNT_FiringPoint!=i)
				glRGB(FS_GUN_CROSSCOLOR_R,FS_GUN_CROSSCOLOR_G,FS_GUN_CROSSCOLOR_B);
			else
			{
				glRGB(255,255,255);
				mult=2;
			}

			glBegin(GL_LINES);
			DrawCross(gunpnt,mult*FS_GUN_CROSSLENGTH);
			/*glEnd();

			glRGB(255,255,0);
			glBegin(GL_LINES);*/
			glVertex3f(gunpnt.x,gunpnt.y,gunpnt.z);
			glVertex3f(gunpnt.x+gunnrm.x*EDITORFS_REALGUNS_NORMLENGTH,gunpnt.y+gunnrm.y*EDITORFS_REALGUNS_NORMLENGTH,gunpnt.z+gunnrm.z*EDITORFS_REALGUNS_NORMLENGTH);
			glEnd();
		}
	}


#define EDITORFS_EYEPOINT_NORMLENGTH	1000
#define EDITORFS_AUTOCENTER_LENGTH		100000
#define EDITORFS_LIGHTS_CROSSLENGTH		20
	if(m_EditorFS_ShowModelInfo)
	{
		//Techroom center
		FS_VPNT acenpnt=GetDocument()->m_EditorFS_Model.ACenPoint;
		glDisable(GL_TEXTURE_2D);
		glRGB(64,64,64); //gray
		glBegin(GL_LINES);
		DrawCross(acenpnt,EDITORFS_AUTOCENTER_LENGTH);
		glEnd();

		//Mass center
		FS_VPNT mascpnt=GetDocument()->m_EditorFS_Model.mass_center;
		glDisable(GL_TEXTURE_2D);
		glRGB(0,0,128); //dark blue
		glBegin(GL_LINES);
		DrawCross(mascpnt,EDITORFS_AUTOCENTER_LENGTH);
		glEnd();

		//EyePoint stuff
		if(GetDocument()->m_EditorFS_Model.num_eye_points!=0)
		{
			FS_VPNT eyepnt=GetDocument()->m_EditorFS_Model.eye_point_sobj_offset;
			FS_VPNT eyenrm=GetDocument()->m_EditorFS_Model.eye_point_normal;
			int submodel=GetDocument()->m_EditorFS_Model.eye_point_sobj_number;
			//ASSERT(submodel<MAX_FS_SOBJ);
			if(submodel>0 && submodel<MAX_FS_SOBJ)
			{
				eyepnt.x+=pDoc->m_FS_SOBJ[submodel].offset.x;
				eyepnt.y+=pDoc->m_FS_SOBJ[submodel].offset.y;
				eyepnt.z+=pDoc->m_FS_SOBJ[submodel].offset.z;
			}
			glPushMatrix();
			FS_RotateParts(submodel);
			glDisable(GL_TEXTURE_2D);
			glRGB(255,0,0); //red
			glBegin(GL_LINES);
			DrawCross(eyepnt,FS_GUN_CROSSLENGTH);
			glVertex3f(eyepnt.x,eyepnt.y,eyepnt.z);
			glVertex3f(eyepnt.x+eyenrm.x*EDITORFS_EYEPOINT_NORMLENGTH,eyepnt.y+eyenrm.y*EDITORFS_EYEPOINT_NORMLENGTH,eyepnt.z+eyenrm.z*EDITORFS_EYEPOINT_NORMLENGTH);
			glEnd();
			glPopMatrix();
		}

		//Lights
		int mult;
		for(int i=0;i<GetDocument()->m_EditorFS_Model.num_lights;i++)
		{
			glDisable(GL_TEXTURE_2D);
			if(i==m_EditorFS_MODEL_CurLight)
			{
				mult=2;
				glRGB(224,255,224); //light green
			}
			else
			{
				mult=1;
				glRGB(0,255,0); //green
			}
			glBegin(GL_LINES);
			DrawCross(GetDocument()->m_EditorFS_Model.light_location[i],mult*EDITORFS_LIGHTS_CROSSLENGTH);
			glEnd();
		}

	}

	


	//Segments stuff
	ASSERT(pDoc->m_FS_NumSOBJ<MAX_FS_SOBJ);
	for (int i=0;i<pDoc->m_FS_NumSOBJ;i++)
	{
		int showthissubmodel=0;
		if(m_ShowSegments && ((pDoc->m_FS_SOBJ[i].detail==(long)m_Detaillevel) &&((m_DisplaySubmodel==-1) || ((int)i==FS_CalcRealSubmodelNumber(m_DisplaySubmodel)))))
			showthissubmodel=1;
		if(i==m_EditorFS_HighLight_Segment_1)
			showthissubmodel=2;
		if(i==m_EditorFS_HighLight_Segment_2)
			showthissubmodel=3;

		if(showthissubmodel>0)
		{
			glPushMatrix();
			FS_RotateParts(i);
			glDisable(GL_TEXTURE_2D);
			switch(showthissubmodel)
			{
			case 1: glRGB(255,  0,  0); break;
			case 2: glRGB(255,127,127); break; // LIGHT RED SEGMENT
			case 3: glRGB(127,127,255); break; // LIGHT BLUE SEGMENT
			default: ASSERT(FALSE);
			}

			FS_VPNT boxpnt1=pDoc->m_FS_SOBJ[i].bounding_box_min_point;
			FS_VPNT boxpnt2=pDoc->m_FS_SOBJ[i].bounding_box_max_point;

			boxpnt1.x+=pDoc->m_FS_SOBJ[i].offset.x;
			boxpnt1.y+=pDoc->m_FS_SOBJ[i].offset.y;
			boxpnt1.z+=pDoc->m_FS_SOBJ[i].offset.z;
			boxpnt2.x+=pDoc->m_FS_SOBJ[i].offset.x;
			boxpnt2.y+=pDoc->m_FS_SOBJ[i].offset.y;
			boxpnt2.z+=pDoc->m_FS_SOBJ[i].offset.z;

			FS_BuildWireCube(boxpnt1,boxpnt2);
			glBegin(GL_LINES);
			for (int j=0;j<24;j++)
				glVertex3fv(FS_WireCube[j]);
			glEnd();
			glPopMatrix();
		}
	}

	//Dock stuff
	if(m_FS_ShowDockingPoints)
	{
		ASSERT(pDoc->m_FS_NumDocks<MAX_FS_DOCKS);
		for (int i=0;i<pDoc->m_FS_NumDocks;i++)
		{
			for(int m=0;m<pDoc->m_FS_Docks[i].num_points;m++)
			{
				glDisable(GL_TEXTURE_2D);
				glRGB(0,255,0);

				vms_vector pnt1=pDoc->m_FS_Docks[i].position[m];
				vms_vector pnt2=pDoc->m_FS_Docks[i].normal[m];
				pnt2.x*=100; pnt2.y*=100; pnt2.z*=100;
				pnt2.x+=pnt1.x;	pnt2.y+=pnt1.y;	pnt2.z+=pnt1.z;

				glBegin(GL_LINES);
				glVertex3f(pnt1.x,pnt1.y,pnt1.z);
				glVertex3f(pnt2.x,pnt2.y,pnt2.z);
				glEnd();
			}
		}
	}

	//Shields stuff
	if ((pDoc->m_FS_Model.shields.Fcount>0) && (pDoc->m_FS_Model.shields.Vcount>0) && (m_ShowShield || GetMainFrame()->m_EditorFS_CurrentView==ID_EDITORFS_SHLD))
	{
		glDisable(GL_TEXTURE_2D);
		glRGB(255,255,0);
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

		glBegin(GL_TRIANGLES);

		ASSERT(pDoc->m_FS_Model.shields.Fcount<MAX_FS_SHIELDFACES);
		for (unsigned int j=0;j<pDoc->m_FS_Model.shields.Fcount;j++)
		{
			normal[0]=pDoc->m_FS_Model.shields.Face[j].Normal.x;
			normal[1]=pDoc->m_FS_Model.shields.Face[j].Normal.y;
			normal[2]=pDoc->m_FS_Model.shields.Face[j].Normal.z;
			for (int i=0;i<3;i++)
			{
				v[i][0]=pDoc->m_FS_Model.shields.Vpoint[pDoc->m_FS_Model.shields.Face[j].Vface[i]][0];
				v[i][1]=pDoc->m_FS_Model.shields.Vpoint[pDoc->m_FS_Model.shields.Face[j].Vface[i]][1];
				v[i][2]=pDoc->m_FS_Model.shields.Vpoint[pDoc->m_FS_Model.shields.Face[j].Vface[i]][2];
			}
			glNormal3fv(normal);
			for (int i=0;i<3;i++)
				glVertex3fv(v[i]);
		}
		glEnd();
	}

	//Thruster glow stuff
	ASSERT(pDoc->m_FS_Model.num_thrusters<MAX_FS_THRUSTERS);
	if(pDoc->m_FS_Model.num_thrusters>0 && (m_FS_ShowThrusterGlows || m_FS_DisplayThruster!=-1))
	{
		glDisable(GL_TEXTURE_2D);
		glRGB(255,128,255);
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

		for(int i=0;i<pDoc->m_FS_Model.num_thrusters;i++)
		{
			ASSERT(pDoc->m_FS_Model.thrusters[i].num_glows<MAX_FS_THRUSTERS_GLOWS);

			if(m_FS_DisplayThruster==-1 || m_FS_DisplayThruster==i)
			{
				for(int j=0;j<pDoc->m_FS_Model.thrusters[i].num_glows;j++)
				{
					FS_VPNT p=pDoc->m_FS_Model.thrusters[i].glow_pos[j];
					FS_VPNT m=pDoc->m_FS_Model.thrusters[i].glow_norm[j];
					float r=pDoc->m_FS_Model.thrusters[i].glow_radius[j];

					//EditorFS
					if(m_FS_DisplayThruster==i)
					{
						p=pDoc->m_EditorFS_FUEL.glow_pos[j];
						m=pDoc->m_EditorFS_FUEL.glow_norm[j];
						r=pDoc->m_EditorFS_FUEL.glow_radius[j];
					}

					glBegin(GL_LINES);
					//DrawCross(p,r);
					glVertex3f(p.x-r,p.y,p.z);
					glVertex3f(p.x+r,p.y,p.z);
					glVertex3f(p.x,p.y-r,p.z);
					glVertex3f(p.x,p.y+r,p.z);
					glVertex3f(p.x,p.y,p.z);
					glVertex3f(p.x+m.x,p.y+m.y,p.z+m.z);
					glVertex3f(p.x-r,p.y,p.z);
					glVertex3f(p.x+m.x,p.y+m.y,p.z+m.z);
					glVertex3f(p.x+r,p.y,p.z);
					glVertex3f(p.x+m.x,p.y+m.y,p.z+m.z);
					glVertex3f(p.x,p.y-r,p.z);
					glVertex3f(p.x+m.x,p.y+m.y,p.z+m.z);
					glVertex3f(p.x,p.y+r,p.z);
					glVertex3f(p.x+m.x,p.y+m.y,p.z+m.z);
					glEnd();
				}
			}
		}
	}

	//Now render the model itself
	if(SmartRenderEngine_RenderMode()!=RENDER_WIREFRAME)
		glPolygonMode(GL_FRONT,GL_FILL);
	else
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	 
	ASSERT(pDoc->m_FS_Model.Pcount<MAX_FS_POLYGONS);
	ASSERT(pDoc->m_FS_Model.Vcount<MAX_FS_VERTICES);
	ASSERT(pDoc->m_FS_Model.Ncount<MAX_FS_NORMALS);
	for(unsigned int j=0;j<pDoc->m_FS_Model.Pcount;j++)
	{			   // type 2 polygons
		if((pDoc->m_FS_Model.Poly[j].Ptype==2)&(pDoc->m_FS_SOBJ[pDoc->m_FS_Model.Poly[j].Sobj].detail==(long)m_Detaillevel))
		{
			glDisable(GL_TEXTURE_2D);

			if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
			{
				glRGB(((pDoc->m_FS_Model.Poly[j].Colors)&0xff),
					   ((pDoc->m_FS_Model.Poly[j].Colors>>8)&0xff),
					   ((pDoc->m_FS_Model.Poly[j].Colors>>16)&0xff));
			} else
				glRGB(192,192,192);

			glBegin(GL_POLYGON);

			normal[0]=pDoc->m_FS_Model.Poly[j].Normal.x;
			normal[1]=pDoc->m_FS_Model.Poly[j].Normal.y;
			normal[2]=pDoc->m_FS_Model.Poly[j].Normal.z;

			ASSERT(pDoc->m_FS_Model.Poly[j].Corners<35);
			for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
			{
				v[i][0]=pDoc->m_FS_Model.Vpoint[pDoc->m_FS_Model.Poly[j].Vp[i]].x;
				v[i][1]=pDoc->m_FS_Model.Vpoint[pDoc->m_FS_Model.Poly[j].Vp[i]].y;
				v[i][2]=pDoc->m_FS_Model.Vpoint[pDoc->m_FS_Model.Poly[j].Vp[i]].z;
			}
			if(m_RenderSmooth)
			{
				for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
				{
					n[i][0]=pDoc->m_FS_Model.Npoint[pDoc->m_FS_Model.Poly[j].Np[i]].x;
					n[i][1]=pDoc->m_FS_Model.Npoint[pDoc->m_FS_Model.Poly[j].Np[i]].y;
					n[i][2]=pDoc->m_FS_Model.Npoint[pDoc->m_FS_Model.Poly[j].Np[i]].z;
				}
			} else
				glNormal3fv(normal);

			for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
			{
				if(m_RenderSmooth)
					glNormal3fv(n[i]);
				glVertex3fv(v[i]);
			}
			glEnd();
		}
	}

	//
	// end of the type 2 stuff...
	//
	// start the type 3 stuff...
	//
	if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);

	ASSERT (m_Detaillevel < 200);
	if (m_Detaillevel >= 200) 
		return; //Sanity check
	for(unsigned int k=pDoc->m_FS_PofDataL[m_Detaillevel]; (k<(pDoc->m_FS_PofDataH[m_Detaillevel]+1)) && (k < MAX_FS_TEXTURE) ;k++)
	{
		if((m_DisplayTexture==-1) | (m_DisplayTexture==(int)(k-pDoc->m_FS_PofDataL[m_Detaillevel])))
		{
			ValidTexture=FALSE;
			if(k<pDoc->m_FS_BitmapData.count)
			{
				if((pDoc->m_FS_BitmapData.pic[k].valid==1) || (SmartRenderEngine_RenderMode()!=RENDER_TEXTURED))
				{
					ValidTexture=TRUE;
					if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
					{
						glRGB(255,255,255);
						glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
						for(unsigned int xv=0;xv<pDoc->m_FS_BitmapData.count;xv++) {
							TRACE("%i,%i|m_FS_LoadPCX[%i]=%i\n",pDoc->m_FS_BitmapData.pic[xv].valid,pDoc->m_FS_BitmapData.count,xv,pDoc->m_FS_LoadPCX[xv]);
						}
//TRACE("%i|m_FS_LoadPCX[%i]=%i\n",pDoc->m_FS_BitmapData.count,k,pDoc->m_FS_LoadPCX[k]);
						ASSERT (pDoc->m_FS_LoadPCX[k] < MAX_FS_TEXTURE);
						if (pDoc->m_FS_LoadPCX[k] >= MAX_FS_TEXTURE) return; //More sanity checking
						glCallList(pDoc->m_FS_ModelTexture[pDoc->m_FS_LoadPCX[k]]);
					} else
						glRGB(192,192,192);
					
				}
			}

			if(ValidTexture)
			{
				for(unsigned int j=0;j<pDoc->m_FS_Model.Pcount;j++)
				{			   // type 3 polygons
					if((pDoc->m_FS_Model.Poly[j].Ptype==3)
						&(pDoc->m_FS_SOBJ[pDoc->m_FS_Model.Poly[j].Sobj].detail==(long)m_Detaillevel)
						&(pDoc->m_FS_Model.Poly[j].Colors==k))
					{
						if((m_DisplaySubmodel==-1) | (FS_CalcRealSubmodelNumber(m_DisplaySubmodel)==(int)pDoc->m_FS_Model.Poly[j].Sobj))
						{
							glPushMatrix();
							FS_RotateParts(pDoc->m_FS_Model.Poly[j].Sobj);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
							glBegin(GL_POLYGON);

							normal[0]=pDoc->m_FS_Model.Poly[j].Normal.x;
							normal[1]=pDoc->m_FS_Model.Poly[j].Normal.y;
							normal[2]=pDoc->m_FS_Model.Poly[j].Normal.z;

							for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
							{
								v[i][0]=pDoc->m_FS_Model.Vpoint[pDoc->m_FS_Model.Poly[j].Vp[i]].x;
								v[i][1]=pDoc->m_FS_Model.Vpoint[pDoc->m_FS_Model.Poly[j].Vp[i]].y;
								v[i][2]=pDoc->m_FS_Model.Vpoint[pDoc->m_FS_Model.Poly[j].Vp[i]].z;
							}
							if(m_RenderSmooth)
							{
								for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
								{
									n[i][0]=pDoc->m_FS_Model.Npoint[pDoc->m_FS_Model.Poly[j].Np[i]].x;
									n[i][1]=pDoc->m_FS_Model.Npoint[pDoc->m_FS_Model.Poly[j].Np[i]].y;
									n[i][2]=pDoc->m_FS_Model.Npoint[pDoc->m_FS_Model.Poly[j].Np[i]].z;
								}
							}

							if(m_ShowThruster)
							{
								for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
								{
									uv[i][0]=pDoc->m_FS_Model.Poly[j].U[i] *
											((float)pDoc->m_FS_BitmapData.pic[k].xreal/(float)pDoc->m_FS_BitmapData.pic[k].xsize);
									uv[i][1]=pDoc->m_FS_Model.Poly[j].V[i] *
											((float)pDoc->m_FS_BitmapData.pic[k].yreal/(float)pDoc->m_FS_BitmapData.pic[k].ysize);
								}
							} else {
								for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
								{
									uv[i][0]=pDoc->m_FS_Model.Poly[j].U[i];
									uv[i][1]=pDoc->m_FS_Model.Poly[j].V[i];
								}
							}
							if (!m_RenderSmooth)
								glNormal3fv(normal);
							for(unsigned int i=0;i<pDoc->m_FS_Model.Poly[j].Corners;i++)
							{
								if (m_RenderSmooth)
									glNormal3fv(n[i]);
								glTexCoord2fv(uv[i]);
								glVertex3fv(v[i]);
							}
							glEnd();
							glPopMatrix();
						}						
					}
				}
			}
		}
	}
	glEndList();

	if(m_RenderCull)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	//glEnable(GL_POLYGON_SMOOTH);
}

GLint CMODVIEW32View::GetZbufferConst()
{
	if(m_RenderZbuffer)
		return GL_LINEAR;
	else
		return GL_NEAREST;
}


void CMODVIEW32View::OnTimer(UINT nIDEvent) 
{
	switch(nIDEvent)
	{
	case 3:
		if(m_DoRotation && GetDocument()->m_ModelLoaded)
		{
			m_Rotation+=10;
			if(m_Rotation>=360)
				m_Rotation=0;
			InvalidateRect(NULL);
		}
	}
}

void CMODVIEW32View::ChangeSize(int w, int h)
{

	CMODVIEW32Doc *pDoc=(CMODVIEW32Doc *)GetDocument();
	ASSERT_VALID(pDoc);

	if(pDoc->m_ModelLoaded)	{
		if (pDoc->m_Game == GAME_FS) {
			if(pDoc->bSize)
			{
				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				nRange=(float)(pDoc->m_FS_Model.MaxXYZ*1.0/*1.1*/);
			} else
				nRange=1500.0f;		
		}
		else ASSERT(FALSE);
	}


	// Prevent a divide by zero
	if(h==0)
		h=1;

	CDataSafe::m_LastWidth=w;
	CDataSafe::m_LastHeight=h;

	// Set Viewport to window dimensions
	glViewport(0,0,w,h);

	// Reset coordinate system
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Establish clipping volume (left, right, bottom, top, near, far)
	if(w<=h)
		glOrtho(-nRange,nRange,-nRange*h/w,nRange*h/w,-nRange*5,nRange*5);
	else
		glOrtho(-nRange*w/h,nRange*w/h,-nRange,nRange,-nRange*5,nRange*5);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void CMODVIEW32View::ChangeSize()
{
	ChangeSize(CDataSafe::m_LastWidth, CDataSafe::m_LastHeight);
//	InvalidateRect(NULL);
}


void CMODVIEW32View::QuickRenderMode_ChangeToNormal()
{
	if(m_QuickRenderMode==QUICKRENDERMODE_QUICK)
	{
		m_QuickRenderMode=QUICKRENDERMODE_TONORMAL;
		InvalidateRect(NULL);
	}
}

void CMODVIEW32View::QuickRenderMode_ChangeToQuick()
{
	if(m_QuickRenderMode==QUICKRENDERMODE_NORMAL)
	{
		m_QuickRenderMode=QUICKRENDERMODE_TOQUICK;
		InvalidateRect(NULL);
	}
}

BOOL CMODVIEW32View::SmartRenderEngine_SkipBuild()
{
	if(!GetMainFrame()->m_QuickRendering)
		return m_SkipBuild;
	if(m_QuickRenderMode==QUICKRENDERMODE_TOQUICK)
	{
		m_QuickRenderMode=QUICKRENDERMODE_QUICK;
		return FALSE;
	}
	if(m_QuickRenderMode==QUICKRENDERMODE_TONORMAL)
	{
		m_QuickRenderMode=QUICKRENDERMODE_NORMAL;
		return FALSE;
	}
	if(!m_SkipBuild)
		return FALSE;
	return TRUE;
}

int CMODVIEW32View::SmartRenderEngine_RenderMode()
{
	if(!GetMainFrame()->m_QuickRendering)
		return m_RenderMode;
	if(m_QuickRenderMode==QUICKRENDERMODE_QUICK)
		return RENDER_WIREFRAME;
	return m_RenderMode;
}

void CMODVIEW32View::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_MouseMovePos=point;
	if(GetMainFrame()->m_SwapMouseButtons)
		m_MouseMoveMode=MOUSEMOVEMODE_ZOOM;
	else
		m_MouseMoveMode=MOUSEMOVEMODE_ROTATE;
	if(nFlags & MK_SHIFT)
		m_MouseMoveMode=MOUSEMOVEMODE_PANE;
	QuickRenderMode_ChangeToQuick();
	CView::OnLButtonDown(nFlags, point);
}

void CMODVIEW32View::OnRButtonDown(UINT nFlags, CPoint point) 
{
	m_MouseMovePos=point;
	if(GetMainFrame()->m_SwapMouseButtons)
		m_MouseMoveMode=MOUSEMOVEMODE_ROTATE;
	else
		m_MouseMoveMode=MOUSEMOVEMODE_ZOOM;
	if(nFlags & MK_SHIFT)
		m_MouseMoveMode=MOUSEMOVEMODE_PANE;
	QuickRenderMode_ChangeToQuick();
	CView::OnRButtonDown(nFlags, point);
}

void CMODVIEW32View::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_MouseMoveMode=MOUSEMOVEMODE_NONE;
	QuickRenderMode_ChangeToNormal();
	CView::OnLButtonUp(nFlags, point);
}

void CMODVIEW32View::OnRButtonUp(UINT nFlags, CPoint point) 
{
	m_MouseMoveMode=MOUSEMOVEMODE_NONE;
	QuickRenderMode_ChangeToNormal();
	CView::OnRButtonUp(nFlags, point);
}

void CMODVIEW32View::OnMouseMove(UINT nFlags, CPoint point) 
{
	if(nFlags==0)
	{
		QuickRenderMode_ChangeToNormal();
		m_MouseMoveMode=MOUSEMOVEMODE_NONE;
	}
	float ZoomFactor=0;
	CMODVIEW32Doc *pDoc=(CMODVIEW32Doc *)GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame *viewFrame=static_cast<CMainFrame*>(GetParentFrame());

	switch(m_MouseMoveMode)
	{
	case MOUSEMOVEMODE_NONE:
		break;

	case MOUSEMOVEMODE_ROTATE:
		m_yRotation-=(float)(m_MouseMovePos.x-point.x)/2.0f;
		m_xRotation-=(float)(m_MouseMovePos.y-point.y)/2.0f;
		m_MouseMovePos=point;
		if(m_yRotation< 0)
			m_yRotation=m_yRotation+360;
		if(m_yRotation>=360)
			m_yRotation=m_yRotation-360;
		if(m_xRotation< 0)
			m_xRotation=m_xRotation+360;
		if(m_xRotation>=360)
			m_xRotation=m_xRotation-360;
		m_SkipBuild=TRUE;
		InvalidateRect(NULL,FALSE);
		viewFrame->m_YRotation=m_yRotation;
		viewFrame->m_XRotation=m_xRotation;
		break;

	case MOUSEMOVEMODE_ZOOM:
		//m_zTranslation+=(float)(m_MouseMovePos.y-point.y)*(GetMainFrame()->CalcPaneDrag())/2.0f;
		ZoomFactor=(float)(m_MouseMovePos.y-point.y)/100.0f;	
		m_xScaling+=ZoomFactor;
		m_yScaling+=ZoomFactor;
		m_zScaling+=ZoomFactor;
		if(m_xScaling<0.01f)
			m_xScaling=0.01f;
		if(m_yScaling<0.01f)
			m_yScaling=0.01f;
		if(m_zScaling<0.01f)
			m_zScaling=0.01f;
		m_MouseMovePos=point;
		m_SkipBuild=TRUE;
		InvalidateRect(NULL,FALSE);
		viewFrame->m_ZoomLevel=m_xScaling;
		break;

	case MOUSEMOVEMODE_PANE:
		m_xTranslation-=(float)(m_MouseMovePos.x-point.x)*(GetMainFrame()->CalcPaneDrag())/2.0f;
		m_yTranslation+=(float)(m_MouseMovePos.y-point.y)*(GetMainFrame()->CalcPaneDrag())/2.0f;
		m_MouseMovePos=point;
		m_SkipBuild=TRUE;
		InvalidateRect(NULL,FALSE);
		break;

	default:
		ASSERT(FALSE);
	}

	CView::OnMouseMove(nFlags, point);
}

//Draw axes
void CMODVIEW32View::DrawCartAxis()
{
	
#define XAXIS_COLOR 0.5f,0.0f,0.0f
#define YAXIS_COLOR 0.0f,0.5f,0.0f
#define ZAXIS_COLOR 0.0f,0.0f,0.5f
#define AXIS_LENGTH 1.0f


	glPushMatrix();
	glNewList(Axis=glGenLists(1), GL_COMPILE);	
	glBegin(GL_LINES);
		
	// x axis arrow
	glColor3f(XAXIS_COLOR);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(AXIS_LENGTH,0.0f,0.0f);
	
	// y axis arrow
	glColor3f(YAXIS_COLOR);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(0.0f,AXIS_LENGTH,0.0f);
	
	// z axis arrow
	glColor3f(ZAXIS_COLOR);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(0.0f,0.0f,AXIS_LENGTH);

	// x letter & arrowhead
	glColor3f(XAXIS_COLOR);
	glVertex3f(AXIS_LENGTH+0.1f,0.1f,0.0f);
	glVertex3f(AXIS_LENGTH+0.3f,-0.1f,0.0f);
	glVertex3f(AXIS_LENGTH+0.3f,0.1f,0.0f);
	glVertex3f(AXIS_LENGTH+0.1f,-0.1f,0.0f);
	glVertex3f(AXIS_LENGTH+0.0f,0.0f,0.0f);
	glVertex3f(AXIS_LENGTH-0.1f,0.1f,0.0f);
	glVertex3f(AXIS_LENGTH+0.0f,0.0f,0.0f);
	glVertex3f(AXIS_LENGTH-0.1f,-0.1f,0.0f);
	
	// y letter & arrowhead
	glColor3f(YAXIS_COLOR);
	glVertex3f(-0.1f,AXIS_LENGTH+0.3f,0.0f);
	glVertex3f(0.f,AXIS_LENGTH+0.2f,0.0f);
	glVertex3f(0.1f,AXIS_LENGTH+0.3f,0.0f);
	glVertex3f(0.f,AXIS_LENGTH+0.2f,0.0f);
	glVertex3f(0.f,AXIS_LENGTH+0.2f,0.0f);
	glVertex3f(0.f,AXIS_LENGTH+0.1f,0.0f);
	glVertex3f(0.0f,AXIS_LENGTH+0.0f,0.0f);
	glVertex3f(0.1f,AXIS_LENGTH-0.1f,0.0f);
	glVertex3f(0.0f,AXIS_LENGTH+0.0f,0.0f);
	glVertex3f(-0.1f,AXIS_LENGTH-0.1f,0.0f);

	// z letter & arrowhead
	glColor3f(ZAXIS_COLOR);		
	glVertex3f(0.0f,-0.1f,AXIS_LENGTH+0.3f);
	glVertex3f(0.0f,-0.1f,AXIS_LENGTH+0.1f);
	glVertex3f(0.0f,-0.1f,AXIS_LENGTH+0.1f);
	glVertex3f(0.0f,0.1f,AXIS_LENGTH+0.3f);
	glVertex3f(0.0f,0.1f,AXIS_LENGTH+0.3f);
	glVertex3f(0.0f,0.1f,AXIS_LENGTH+0.1f);
	glVertex3f(0.0f,0.0f,AXIS_LENGTH+0.0f);
	glVertex3f(0.0f,0.1f,AXIS_LENGTH-0.1f);
	glVertex3f(0.0f,0.0f,AXIS_LENGTH+0.0f);
	glVertex3f(0.0f,-0.1f,AXIS_LENGTH-0.1f);

	glEnd();

	glEndList();
}


void CMODVIEW32View::RenderIndicator()
{
	if(!GetDocument()->m_ModelLoaded)
		return;
	DrawCartAxis();
	glRotatef(m_xRotation, 1.0, 0.0, 0.0);
	glRotatef(m_yRotation, 0.0, 1.0, 0.0);
	glPushMatrix();
	
	//"superimpose" the RGB-carthesian-axes in a small area near the bottom-left corner of the view
	//glViewport(m_ClientRect.right + (m_ClientRect.right/6),0,m_ClientRect.right/6,m_ClientRect.bottom/6);
	glViewport(0,0,m_ClientRect.right/6,m_ClientRect.bottom/6);
	//
	glPopMatrix();
	
	glScalef(10.0,10.0,10.0);

	glPushAttrib(GL_ENABLE_BIT);
	//glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glCallList(Axis);
	glPopAttrib();
	glViewport(0,0,m_ClientRect.right,m_ClientRect.bottom);
	glPopMatrix();
}


//int CMODVIEW32View::D2_BuildScene()
//{
//	CMODVIEW32Doc* pDoc=GetDocument();
//
//	int i,j,k;
//	float v[35][3];
//	float uv[35][2];
//	float normal[3];
//	BOOL data2type, data3type;
//
//	data2type=FALSE;
//	data3type=FALSE;
//
//	// Build display list....
//    glClearColor(m_ClearColorRed, m_ClearColorGreen, m_ClearColorBlue, 1.0f);
//	glNewList(m_D2_TestModel=glGenLists(1), GL_COMPILE);
//	//glEnable(GL_TEXTURE_2D);
//	glScalef(-1.0,1.0,1.0);
//	glDisable(GL_TEXTURE_GEN_S);
//	glDisable(GL_TEXTURE_GEN_T);
//	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//	//glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);
//
//	// Rotation time... Look out!
//	for (i=0;i<pDoc->m_D2_Model.Vcount;i++)
//	{
//		pDoc->m_D2_Model.Vpoint[i].x=pDoc->m_D2_Model.Vbackup[i].x;
//		pDoc->m_D2_Model.Vpoint[i].y=pDoc->m_D2_Model.Vbackup[i].y;
//		pDoc->m_D2_Model.Vpoint[i].z=pDoc->m_D2_Model.Vbackup[i].z;
//	}
//	
//	ASSERT(pDoc->m_D2_Model.PolyModel.n_models<MAX_D2_SUBMODELS+1);
//	for (i=1;i<pDoc->m_D2_Model.PolyModel.n_models;i++)
//	{
//		CMathStuff cms;
//		cms.Rotate_Object(i,i,m_D2_PosAngle_Current[i].p,m_D2_PosAngle_Current[i].h,m_D2_PosAngle_Current[i].b,&pDoc->m_D2_Model);
//
//		k=pDoc->m_D2_Model.PolyModel.submodel_parents[i];
//		ASSERT(k<MAX_D2_SUBMODELS+1);
//		while (k!=0)
//		{
//			ASSERT (k < 10);
//			if (k >= 10) return NULL; //Sanity check
//			cms.Rotate_Object(k,i,m_D2_PosAngle_Current[k].p,m_D2_PosAngle_Current[k].h,m_D2_PosAngle_Current[k].b,&pDoc->m_D2_Model);
//			k=pDoc->m_D2_Model.PolyModel.submodel_parents[k];
//		}
//	}
//
//	//Gun stuff
//	if(m_ShowGuns && pDoc->m_D2_Model.PolType==D2_POLYMODEL_TYPE_ROBOT)
//	{
//		for (i=0;i<pDoc->m_D2_Model.RobotInfo.n_guns;i++)
//		{
//			glDisable(GL_TEXTURE_2D);
//			glRGB(255,255,0);
//
//			D2_VMS_VECTOR gunpnt =pDoc->m_D2_Model.RobotInfo.gun_points[i];
//
//			CMathStuff wc;
//			int subm=pDoc->m_D2_Model.RobotInfo.gun_submodels[i];
//			if (subm!=0)
//			{
//				wc.Rotate_G_Point(subm,m_D2_PosAngle_Current[subm].p,m_D2_PosAngle_Current[subm].h,m_D2_PosAngle_Current[subm].b,&pDoc->m_D2_Model,&gunpnt);
//				k=pDoc->m_D2_Model.PolyModel.submodel_parents[subm];
//				while(k!=0)
//				{
//					wc.Rotate_G_Point(k,m_D2_PosAngle_Current[k].p,m_D2_PosAngle_Current[k].h,m_D2_PosAngle_Current[k].b,&pDoc->m_D2_Model,&gunpnt);
//					k=pDoc->m_D2_Model.PolyModel.submodel_parents[k];
//				}
//			}
//
//			gunpnt.x /= 0x1000;
//			gunpnt.y /= 0x1000;
//			gunpnt.z /= 0x1000;
//
//			if((m_DisplaySubmodel==-1) || (m_DisplaySubmodel==pDoc->m_Guns.InSubModel[i]))
//			{
//				glBegin(GL_LINES);
//				glVertex3i(gunpnt.x+25,gunpnt.y,gunpnt.z);
//				glVertex3i(gunpnt.x-25,gunpnt.y,gunpnt.z);
//				glVertex3i(gunpnt.x,gunpnt.y+25,gunpnt.z);
//				glVertex3i(gunpnt.x,gunpnt.y-25,gunpnt.z);
//				glVertex3i(gunpnt.x,gunpnt.y,gunpnt.z+25);
//				glVertex3i(gunpnt.x,gunpnt.y,gunpnt.z-25);
//				glEnd();
//			}
//		}
//	}
//
//
//	//Segments stuff
//	if(m_ShowSegments)
//	{
//		for (i=0;i<pDoc->m_D2_Model.PolyModel.n_models;i++)
//		{
//			if(m_DisplaySubmodel==-1 || m_DisplaySubmodel==i)
//			{
//				glDisable(GL_TEXTURE_2D);
//				glRGB(255,0,0);
//
//				D2_VMS_VECTOR boxpnt1=pDoc->m_D2_Model.PolyModel.submodel_maxs[i];
//				D2_VMS_VECTOR boxpnt2=pDoc->m_D2_Model.PolyModel.submodel_mins[i];
//
//				CMathStuff wc;
//				wc.D2_BuildWireCube(boxpnt1,boxpnt2);
//
//				if(i!=0)
//				{
//					/*if (bShowMove[i] & (JointListData[JointPos][i].jointnum!=0 ))
//					{
//						Rotate_B_Point(i,
//							JointListData[JointPos][i].angles.p,
//							JointListData[JointPos][i].angles.h,
//							JointListData[JointPos][i].angles.b);
//					} else*/ {
//						wc.Rotate_B_Point(i,m_D2_PosAngle_Current[i].p,m_D2_PosAngle_Current[i].h,m_D2_PosAngle_Current[i].b,&pDoc->m_D2_Model);
//					}
//					
//					k=pDoc->m_D2_Model.PolyModel.submodel_parents[i];
//					while (k!=0)
//					{
//						/*if (bShowMove[k] & (JointListData[JointPos][k].jointnum!=0 ))
//						{
//							Rotate_B_Point(k,
//								JointListData[JointPos][k].angles.p,
//								JointListData[JointPos][k].angles.h,
//								JointListData[JointPos][k].angles.b);
//						} else*/ {
//							wc.Rotate_B_Point(k,m_D2_PosAngle_Current[k].p,m_D2_PosAngle_Current[k].h,m_D2_PosAngle_Current[k].b,&pDoc->m_D2_Model);
//						}
//						k=pDoc->m_D2_Model.PolyModel.submodel_parents[k];
//					}
//				}
//
//				glBegin(GL_LINES);
//				for (j=0;j<24;j++)
//				{
//					wc.D2_WireCube[j][0]/=0x1000;
//					wc.D2_WireCube[j][1]/=0x1000;
//					wc.D2_WireCube[j][2]/=0x1000;
//					glVertex3fv(wc.D2_WireCube[j]);
//				}
//
//				glEnd();
//			}
//		}
//	}
//
//
//	// start of the type 2 stuff...
//	if(m_DisplayTexture==-1)
//	{
//		for (j=1;j<(pDoc->m_D2_Model.Pcount+1);j++)
//		{            // type 2 triangles
//			if((m_DisplaySubmodel==-1) | (m_DisplaySubmodel==pDoc->m_D2_Model.Poly[j].Segment))
//			{
//				if ((pDoc->m_D2_Model.Poly[j].Polytype==2)&(pDoc->m_D2_Model.Poly[j].Corners==3)) 
//				{
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//					if (data2type==FALSE)
//					{
//						glDisable(GL_TEXTURE_2D);
//						data2type=TRUE;
//						glBegin(GL_TRIANGLES);
//					}
//
//					if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED) {
//						glRGB(((pDoc->m_D2_Model.Poly[j].Colors>>10)&0x1f) * 8,
//								((pDoc->m_D2_Model.Poly[j].Colors>>5)&0x1f) * 8,
//								((pDoc->m_D2_Model.Poly[j].Colors)&0x1f) * 8);
//					} else {
//						glRGB(192,192,192);
//					}
//
//					normal[0]=((float)pDoc->m_D2_Model.Poly[j].Normal.x) / 0x10000;
//					normal[1]=((float)pDoc->m_D2_Model.Poly[j].Normal.y) / 0x10000;
//					normal[2]=((float)pDoc->m_D2_Model.Poly[j].Normal.z) / 0x10000;
//
//					for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//						v[i][0]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].x) / 0x1000;
//						v[i][1]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].y) / 0x1000;
//						v[i][2]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].z) / 0x1000;
//					}
//					glNormal3fv(normal);
//					for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//						glVertex3fv(v[i]);
//					}
//				}
//			}
//		}
//		if(data2type)
//		{
//			glEnd();
//			data2type=FALSE;
//		}
//
//		for (j=1;j<(pDoc->m_D2_Model.Pcount+1);j++) {            // type 2 quads
//			if((m_DisplaySubmodel==-1) | (m_DisplaySubmodel==pDoc->m_D2_Model.Poly[j].Segment))
//			{
//				if((pDoc->m_D2_Model.Poly[j].Polytype==2)&(pDoc->m_D2_Model.Poly[j].Corners==4))
//				{
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//					if(data2type==FALSE)
//					{
//						glDisable(GL_TEXTURE_2D);
//						data2type=TRUE;
//						glBegin(GL_QUADS);
//					}
//
//					if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED) {
//						glRGB(((pDoc->m_D2_Model.Poly[j].Colors>>10)&0x1f) * 8,
//								 ((pDoc->m_D2_Model.Poly[j].Colors>>5)&0x1f) * 8,
//								 ((pDoc->m_D2_Model.Poly[j].Colors)&0x1f) * 8);
//					} else {
//						glRGB(192,192,192);
//					}
//
//					normal[0]=((float)pDoc->m_D2_Model.Poly[j].Normal.x) / 0x10000;
//					normal[1]=((float)pDoc->m_D2_Model.Poly[j].Normal.y) / 0x10000;
//					normal[2]=((float)pDoc->m_D2_Model.Poly[j].Normal.z) / 0x10000;
//
//					for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++)
//					{
//						v[i][0]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].x) / 0x1000;
//						v[i][1]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].y) / 0x1000;
//						v[i][2]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].z) / 0x1000;
//					}
//					glNormal3fv(normal);
//					for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++)
//						glVertex3fv(v[i]);
//				}
//			}
//		}
//		if (data2type)
//			glEnd();
//
//		for (j=1;j<(pDoc->m_D2_Model.Pcount+1);j++)
//		{            // type 2 polygons
//			if((m_DisplaySubmodel==-1) | (m_DisplaySubmodel==pDoc->m_D2_Model.Poly[j].Segment))
//			{
//				if((pDoc->m_D2_Model.Poly[j].Polytype==2)&(pDoc->m_D2_Model.Poly[j].Corners>4))
//				{
//					glDisable(GL_TEXTURE_2D);
//					if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
//					{
//						glRGB(((pDoc->m_D2_Model.Poly[j].Colors>>10)&0x1f) * 8,
//								 ((pDoc->m_D2_Model.Poly[j].Colors>>5)&0x1f) * 8,
//								 ((pDoc->m_D2_Model.Poly[j].Colors)&0x1f) * 8);
//					} else
//						glRGB(192,192,192);
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//
//					glBegin(GL_POLYGON);
//
//					normal[0]=((float)pDoc->m_D2_Model.Poly[j].Normal.x) / 0x10000;
//					normal[1]=((float)pDoc->m_D2_Model.Poly[j].Normal.y) / 0x10000;
//					normal[2]=((float)pDoc->m_D2_Model.Poly[j].Normal.z) / 0x10000;
//
//					for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//						v[i][0]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].x) / 0x1000;
//						v[i][1]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].y) / 0x1000;
//						v[i][2]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].z) / 0x1000;
//					}
//					glNormal3fv(normal);
//					for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++)
//						glVertex3fv(v[i]);
//					glEnd();
//				}
//			}
//		}
//	}
//
//	// start the type 3 stuff...
//	if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
//		glEnable(GL_TEXTURE_2D);
//	else
//		glDisable(GL_TEXTURE_2D);
//
//	//Preparation of the DisplayTexture stuff	
//	int m_DisplayTextureB=-1;
//	int m_DisplayTextureC=-1;
//	if(m_DisplayTexture!=-1)
//	{
//		for (i=0;i<pDoc->m_D2_PIGNumTextures;i++)
//		{
//			for (j=0;j<pDoc->m_D2_BitmapData.count;j++)
//			{
//				if (i==pDoc->m_D2_BitmapData.bitmap[j].number)
//				{
//					m_DisplayTextureC++;
//					if(m_DisplayTextureC==m_DisplayTexture)
//						m_DisplayTextureB=j;
//				}
//				
//			}
//		}
//	}
//	
//	//Render type 3 (polygons with textures)
//	for (k=0;k<pDoc->m_D2_BitmapData.count ;k++)
//	{
//		if(m_DisplayTexture==-1 || k==m_DisplayTextureB)
//		{
//
//			if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
//			{
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//				if (pDoc->m_D2_BitmapData.bitmap[k].transparent)
//				{
//					glRGB(0,0,0);
//					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
//					glBlendFunc(GL_SRC_ALPHA,GL_ONE);
//					glEnable(GL_BLEND);
//				} else {
//					glRGB(255,255,255);
//					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
//				}
//				glCallList(pDoc->m_D2_ModelTexture[k]);
//			} else
//				glRGB(192,192,192);
//
//			for (j=1;j<(pDoc->m_D2_Model.Pcount+1);j++)
//			{            // type 3 triangles
//				if((m_DisplaySubmodel==-1) | (m_DisplaySubmodel==pDoc->m_D2_Model.Poly[j].Segment))
//				{
//					if((pDoc->m_D2_Model.Poly[j].Polytype==3)&(pDoc->m_D2_Model.Poly[j].Corners==3)&(pDoc->m_D2_Model.Poly[j].Colors==k))
//					{
//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//						if(data3type==FALSE)
//						{
//							data3type=TRUE;
//							glBegin(GL_TRIANGLES);
//						}
//						normal[0]=((float)pDoc->m_D2_Model.Poly[j].Normal.x) / 0x10000;
//						normal[1]=((float)pDoc->m_D2_Model.Poly[j].Normal.y) / 0x10000;
//						normal[2]=((float)pDoc->m_D2_Model.Poly[j].Normal.z) / 0x10000;
//			
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							v[i][0]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].x) / 0x1000;
//							v[i][1]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].y) / 0x1000;
//							v[i][2]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].z) / 0x1000;
//						}
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							uv[i][0]=((float)pDoc->m_D2_Model.Poly[j].Bitmap[i].x) / (0x10000);
//							uv[i][1]=((float)pDoc->m_D2_Model.Poly[j].Bitmap[i].y) / (0x10000);
//						}
//						glNormal3fv(normal);
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							glTexCoord2fv(uv[i]);
//							glVertex3fv(v[i]);
//						}
//					}
//				}
//			}
//			if (data3type)
//			{
//				glEnd();
//				data3type=FALSE;
//			}
//			for (j=1;j<(pDoc->m_D2_Model.Pcount+1);j++)
//			{            // type 3 quads
//				if((m_DisplaySubmodel==-1) | (m_DisplaySubmodel==pDoc->m_D2_Model.Poly[j].Segment))
//				{
//					if ((pDoc->m_D2_Model.Poly[j].Polytype==3)&(pDoc->m_D2_Model.Poly[j].Corners==4)&(pDoc->m_D2_Model.Poly[j].Colors==k))
//					{
//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//						if (data3type==FALSE)
//						{
//							data3type=TRUE;
//							glBegin(GL_QUADS);
//						}
//						normal[0]=((float)pDoc->m_D2_Model.Poly[j].Normal.x) / 0x10000;
//						normal[1]=((float)pDoc->m_D2_Model.Poly[j].Normal.y) / 0x10000;
//						normal[2]=((float)pDoc->m_D2_Model.Poly[j].Normal.z) / 0x10000;
//			
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							v[i][0]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].x) / 0x1000;
//							v[i][1]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].y) / 0x1000;
//							v[i][2]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].z) / 0x1000;
//						}
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							uv[i][0]=((float)pDoc->m_D2_Model.Poly[j].Bitmap[i].x) / (0x10000);
//							uv[i][1]=((float)pDoc->m_D2_Model.Poly[j].Bitmap[i].y) / (0x10000);
//						}
//						glNormal3fv(normal);
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++)
//						{
//							glTexCoord2fv(uv[i]);
//							glVertex3fv(v[i]);
//						}
//					}
//				}
//			}
//			if (data3type)
//			{
//				glEnd();
//				data3type=FALSE;
//			}
//			for (j=1;j<(pDoc->m_D2_Model.Pcount+1);j++)
//			{
//				// type 3 polygons
//				if((m_DisplaySubmodel==-1) | (m_DisplaySubmodel==pDoc->m_D2_Model.Poly[j].Segment))
//				{
//					if ((pDoc->m_D2_Model.Poly[j].Polytype==3)&(pDoc->m_D2_Model.Poly[j].Corners>4)&(pDoc->m_D2_Model.Poly[j].Colors==k))
//					{
//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetZbufferConst());
//						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetZbufferConst());
//						glBegin(GL_POLYGON);
//						normal[0]=((float)pDoc->m_D2_Model.Poly[j].Normal.x) / 0x10000;
//						normal[1]=((float)pDoc->m_D2_Model.Poly[j].Normal.y) / 0x10000;
//						normal[2]=((float)pDoc->m_D2_Model.Poly[j].Normal.z) / 0x10000;
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							v[i][0]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].x) / 0x1000;
//							v[i][1]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].y) / 0x1000;
//							v[i][2]=((float)pDoc->m_D2_Model.Vpoint[pDoc->m_D2_Model.Poly[j].Pointindex[i]].z) / 0x1000;
//						}
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							uv[i][0]=((float)pDoc->m_D2_Model.Poly[j].Bitmap[i].x) / (0x10000);
//							uv[i][1]=((float)pDoc->m_D2_Model.Poly[j].Bitmap[i].y) / (0x10000);
//						}
//						glNormal3fv(normal);
//						for (i=0;i<pDoc->m_D2_Model.Poly[j].Corners;i++) {
//							glTexCoord2fv(uv[i]);
//							glVertex3fv(v[i]);
//						}
//						glEnd();
//					}
//				}
//			}
//			if(SmartRenderEngine_RenderMode()==RENDER_TEXTURED)
//			{
//				if (pDoc->m_D2_BitmapData.bitmap[k].transparent)
//					glDisable(GL_BLEND);
//			}
//		}
//	}
//
//	glEndList();
//	return 0;
//}
//
//int CMODVIEW32View::D2_RenderScene()
//{
//
//	// Turn culling on if flag is set
//	if(m_RenderCull)
//		glEnable(GL_CULL_FACE);
//	else
//		glDisable(GL_CULL_FACE);
//
//	// Enable Line or fill mode
//	if(SmartRenderEngine_RenderMode()!=RENDER_WIREFRAME)
//		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
//	else
//		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
//
//	// Enable depth testing if flag is set
////	if(bDepth)
////		glEnable(GL_DEPTH_TEST);
////	else
//		//glDisable(GL_DEPTH_TEST);
//
//	// Clear the window with current clearing color
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	// Save the matrix state and do the rotations
//	glPushMatrix();
//	glScalef(-1.0,1.0,1.0); //HHADD
//
//	// Display polymodel
//	glCallList(m_D2_TestModel);
//	glPopMatrix();
//	glMatrixMode(GL_MODELVIEW);
//
//	// Flush drawing commands
//	glFlush();
//	return 0;
//}
//
//void CMODVIEW32View::D2_BuildWireCube(D2_VMS_VECTOR min, D2_VMS_VECTOR max)
//{
///*	D2_WireCube[0][0]=max.x; D2_WireCube[0][1]=max.y; D2_WireCube[0][2]=max.z;
//	D2_WireCube[1][0]=min.x; D2_WireCube[1][1]=max.y; D2_WireCube[1][2]=max.z;
//	D2_WireCube[2][0]=max.x; D2_WireCube[2][1]=max.y; D2_WireCube[2][2]=max.z;
//	D2_WireCube[3][0]=max.x; D2_WireCube[3][1]=min.y; D2_WireCube[3][2]=max.z;
//	D2_WireCube[4][0]=max.x; D2_WireCube[4][1]=max.y; D2_WireCube[4][2]=max.z;
//	D2_WireCube[5][0]=max.x; D2_WireCube[5][1]=max.y; D2_WireCube[5][2]=min.z;
//
//	D2_WireCube[6][0]=min.x; D2_WireCube[6][1]=max.y; D2_WireCube[6][2]=max.z;
//	D2_WireCube[7][0]=min.x; D2_WireCube[7][1]=min.y; D2_WireCube[7][2]=max.z;
//	D2_WireCube[8][0]=min.x; D2_WireCube[8][1]=max.y; D2_WireCube[8][2]=max.z;
//	D2_WireCube[9][0]=min.x; D2_WireCube[9][1]=max.y; D2_WireCube[9][2]=min.z;
//
//	D2_WireCube[10][0]=max.x; D2_WireCube[10][1]=min.y; D2_WireCube[10][2]=max.z;
//	D2_WireCube[11][0]=min.x; D2_WireCube[11][1]=min.y; D2_WireCube[11][2]=max.z;
//	D2_WireCube[12][0]=max.x; D2_WireCube[12][1]=min.y; D2_WireCube[12][2]=max.z;
//	D2_WireCube[13][0]=max.x; D2_WireCube[13][1]=min.y; D2_WireCube[13][2]=min.z;
//
//	D2_WireCube[14][0]=max.x; D2_WireCube[14][1]=max.y; D2_WireCube[14][2]=min.z;
//	D2_WireCube[15][0]=min.x; D2_WireCube[15][1]=max.y; D2_WireCube[15][2]=min.z;
//	D2_WireCube[16][0]=max.x; D2_WireCube[16][1]=max.y; D2_WireCube[16][2]=min.z;
//	D2_WireCube[17][0]=max.x; D2_WireCube[17][1]=min.y; D2_WireCube[17][2]=min.z;
//
//	D2_WireCube[18][0]=max.x; D2_WireCube[18][1]=min.y; D2_WireCube[18][2]=min.z;
//	D2_WireCube[19][0]=min.x; D2_WireCube[19][1]=min.y; D2_WireCube[19][2]=min.z;
//	D2_WireCube[20][0]=min.x; D2_WireCube[20][1]=max.y; D2_WireCube[20][2]=min.z;
//	D2_WireCube[21][0]=min.x; D2_WireCube[21][1]=min.y; D2_WireCube[21][2]=min.z;
//	D2_WireCube[22][0]=min.x; D2_WireCube[22][1]=min.y; D2_WireCube[22][2]=max.z;
//	D2_WireCube[23][0]=min.x; D2_WireCube[23][1]=min.y; D2_WireCube[23][2]=min.z;*/
//}

void CMODVIEW32View::FS_BuildWireCube(FS_VPNT min, FS_VPNT max)
{
	FS_WireCube[0][0]=max.x; FS_WireCube[0][1]=max.y; FS_WireCube[0][2]=max.z;
	FS_WireCube[1][0]=min.x; FS_WireCube[1][1]=max.y; FS_WireCube[1][2]=max.z;
	FS_WireCube[2][0]=max.x; FS_WireCube[2][1]=max.y; FS_WireCube[2][2]=max.z;
	FS_WireCube[3][0]=max.x; FS_WireCube[3][1]=min.y; FS_WireCube[3][2]=max.z;
	FS_WireCube[4][0]=max.x; FS_WireCube[4][1]=max.y; FS_WireCube[4][2]=max.z;
	FS_WireCube[5][0]=max.x; FS_WireCube[5][1]=max.y; FS_WireCube[5][2]=min.z;

	FS_WireCube[6][0]=min.x; FS_WireCube[6][1]=max.y; FS_WireCube[6][2]=max.z;
	FS_WireCube[7][0]=min.x; FS_WireCube[7][1]=min.y; FS_WireCube[7][2]=max.z;
	FS_WireCube[8][0]=min.x; FS_WireCube[8][1]=max.y; FS_WireCube[8][2]=max.z;
	FS_WireCube[9][0]=min.x; FS_WireCube[9][1]=max.y; FS_WireCube[9][2]=min.z;

	FS_WireCube[10][0]=max.x; FS_WireCube[10][1]=min.y; FS_WireCube[10][2]=max.z;
	FS_WireCube[11][0]=min.x; FS_WireCube[11][1]=min.y; FS_WireCube[11][2]=max.z;
	FS_WireCube[12][0]=max.x; FS_WireCube[12][1]=min.y; FS_WireCube[12][2]=max.z;
	FS_WireCube[13][0]=max.x; FS_WireCube[13][1]=min.y; FS_WireCube[13][2]=min.z;

	FS_WireCube[14][0]=max.x; FS_WireCube[14][1]=max.y; FS_WireCube[14][2]=min.z;
	FS_WireCube[15][0]=min.x; FS_WireCube[15][1]=max.y; FS_WireCube[15][2]=min.z;
	FS_WireCube[16][0]=max.x; FS_WireCube[16][1]=max.y; FS_WireCube[16][2]=min.z;
	FS_WireCube[17][0]=max.x; FS_WireCube[17][1]=min.y; FS_WireCube[17][2]=min.z;

	FS_WireCube[18][0]=max.x; FS_WireCube[18][1]=min.y; FS_WireCube[18][2]=min.z;
	FS_WireCube[19][0]=min.x; FS_WireCube[19][1]=min.y; FS_WireCube[19][2]=min.z;
	FS_WireCube[20][0]=min.x; FS_WireCube[20][1]=max.y; FS_WireCube[20][2]=min.z;
	FS_WireCube[21][0]=min.x; FS_WireCube[21][1]=min.y; FS_WireCube[21][2]=min.z;
	FS_WireCube[22][0]=min.x; FS_WireCube[22][1]=min.y; FS_WireCube[22][2]=max.z;
	FS_WireCube[23][0]=min.x; FS_WireCube[23][1]=min.y; FS_WireCube[23][2]=min.z;
}

int CMODVIEW32View::FS_CalcRealSubmodelNumber(int subm)
{
	if(subm!=-1)
	{
		int num_sobj=-1;
		for(int j=0;j<GetDocument()->m_FS_NumSOBJ;j++)
		{	
			if(GetDocument()->m_FS_SOBJ[j].detail==(long)m_Detaillevel)
			{
				num_sobj++;
				if(num_sobj==subm)
					return j;
			}
		}
	}
	return -1;
}

void CMODVIEW32View::DrawCross(FS_VPNT point, int len)
{
	glVertex3f(point.x+len,point.y,point.z);
	glVertex3f(point.x-len,point.y,point.z);
	glVertex3f(point.x,point.y+len,point.z);
	glVertex3f(point.x,point.y-len,point.z);
	glVertex3f(point.x,point.y,point.z+len);
	glVertex3f(point.x,point.y,point.z-len);
}


void CMODVIEW32View::OnKeyDown2(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
	//Processing Arrow Keys to Synchronise with Movement
	case VK_UP:
		m_DirZ = m_PosZ - 0.5f*(GLfloat)sin(m_AngleX*PI/180.0f);
		m_PosZ = m_PosZ - m_PosIncr*(GLfloat)sin(m_AngleX*PI/180.0f);
		m_DirX = m_PosX - 0.5f*(GLfloat)cos(m_AngleX*PI/180.0f);
		m_PosX = m_PosX - m_PosIncr*(GLfloat)cos(m_AngleX*PI/180.0f);
		InvalidateRect(NULL,FALSE);
		break;
	case VK_DOWN:
		m_DirZ = m_PosZ - 0.5f*(GLfloat)sin(m_AngleX*PI/180.0f);
		m_PosZ = m_PosZ + m_PosIncr*(GLfloat)sin(m_AngleX*PI/180.0f);
		m_DirX = m_PosX - 0.5f*(GLfloat)cos(m_AngleX*PI/180.0f);
		m_PosX = m_PosX + m_PosIncr*(GLfloat)cos(m_AngleX*PI/180.0f);
		InvalidateRect(NULL,FALSE);
		break;
	case VK_LEFT:
		m_AngleX = m_AngleX - m_AngIncr;
		m_DirZ = m_PosZ - 0.5f*(GLfloat)sin(m_AngleX*PI/180.0f);
		m_DirX = m_PosX - 0.5f*(GLfloat)cos(m_AngleX*PI/180.0f);
		InvalidateRect(NULL,FALSE);
		break;
	case VK_RIGHT:
		m_AngleX = m_AngleX + m_AngIncr;	
		m_DirZ = m_PosZ - 0.5f*(GLfloat)sin(m_AngleX*PI/180.0f);
		m_DirX = m_PosX - 0.5f*(GLfloat)cos(m_AngleX*PI/180.0f);
		InvalidateRect(NULL,FALSE);
		break;
	case 'A':
		m_PosY = m_PosY + m_PosIncr;
		m_DirY = m_PosY;
		InvalidateRect(NULL,FALSE);
		break;
	case 'Y':
		m_PosY = m_PosY - m_PosIncr;
		m_DirY = m_PosY;
		InvalidateRect(NULL,FALSE);
		break;
	}
	
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMODVIEW32View::CopyToClipboard()
{
	BeginWaitCursor(); 

	// Get client geometry 
	CRect rect; 
	GetClientRect(&rect); 
	CSize size(rect.Width(),rect.Height()); 
	TRACE("  client zone : (%d;%d)\n",size.cx,size.cy); 
	// Lines have to be 32 bytes aligned, suppose 24 bits per pixel 
	// I just cropped it 
	CDC *pDC = GetDC(); 
	int x=pDC->GetDeviceCaps(BITSPIXEL);
	switch(pDC->GetDeviceCaps(BITSPIXEL))
	{
	case 8:
	case 24: size.cx-=size.cx%4; break;
	case 16: size.cx-=size.cx%4/*2?*/; break;
	case 32: 
	default: break;
	}
	TRACE("  final client zone : (%d;%d)\n",size.cx,size.cy); 

	// Create a bitmap and select it in the device context 
	// Note that this will never be used ;-) but no matter 
	CBitmap bitmap; 
	CDC MemDC; 
	VERIFY(MemDC.CreateCompatibleDC(NULL)); 
	VERIFY(bitmap.CreateCompatibleBitmap(pDC,size.cx,size.cy)); 
	MemDC.SelectObject(&bitmap); 

	// Alloc pixel bytes 
	int NbBytes=3*size.cx*size.cy; 
	unsigned char *pPixelData=new unsigned char[NbBytes]; 

	// Copy from OpenGL 
	::glReadPixels(0,0,size.cx,size.cy,GL_BGR_EXT,GL_UNSIGNED_BYTE,pPixelData); 

	// Fill header 
	BITMAPINFOHEADER header; 
	header.biWidth=size.cx; 
	header.biHeight=size.cy; 
	header.biSizeImage=NbBytes; 
	header.biSize=40; 
	header.biPlanes=1; 
	header.biBitCount=3*8; // RGB 
	header.biCompression=0; 
	header.biXPelsPerMeter=0; 
	header.biYPelsPerMeter=0; 
	header.biClrUsed=0; 
	header.biClrImportant=0; 

	// Generate handle 
	HANDLE handle=(HANDLE)::GlobalAlloc(GHND,sizeof(BITMAPINFOHEADER)+NbBytes); 
	if(handle!=NULL) 
	{ 
		// Lock handle 
		char *pData=(char *)::GlobalLock((HGLOBAL)handle); 
		// Copy header and data 
		if ( pData != NULL) {
			memcpy(pData,&header,sizeof(BITMAPINFOHEADER)); 
			memcpy(pData+sizeof(BITMAPINFOHEADER),pPixelData,NbBytes); 
			// Unlock 
			::GlobalUnlock((HGLOBAL)handle); 

			// Push DIB in clipboard 
			OpenClipboard(); 
			EmptyClipboard(); 
			SetClipboardData(CF_DIB,handle); 
			CloseClipboard(); 
		}
	}

	// Cleanup 
	MemDC.DeleteDC(); 
	bitmap.DeleteObject(); 
	delete []pPixelData; 
	
	EndWaitCursor(); 
}

void CMODVIEW32View::FS_RotateParts(int subm)
{
	if(!m_DoRotation)
		return;
	if(m_Rotation==0)
		return;
	
	CMODVIEW32Doc *pDoc=GetDocument();
	if(pDoc->m_FS_SOBJ[subm].movement_type==1 && m_DoRotation && m_Rotation!=0)
	{
		glTranslatef(pDoc->m_FS_SOBJ[subm].offset.x,pDoc->m_FS_SOBJ[subm].offset.y,pDoc->m_FS_SOBJ[subm].offset.z);
		switch(pDoc->m_FS_SOBJ[subm].movement_axis)
		{
		case 0: glRotatef(m_Rotation,1.0,0.0,0.0); break;
		case 1: glRotatef(m_Rotation,0.0,0.0,1.0); break;
		case 2: glRotatef(m_Rotation,0.0,1.0,0.0); break;
		}
		glTranslatef(-pDoc->m_FS_SOBJ[subm].offset.x,-pDoc->m_FS_SOBJ[subm].offset.y,-pDoc->m_FS_SOBJ[subm].offset.z);
	}
}