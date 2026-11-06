/////////////////////////////////////////////////////////////////////////////
// Name:        wx/glcanvas.h (OpenCPN on AmigaOS 3, GL off)
// Purpose:     OpenCPN's headers include wx/glcanvas.h whether or not it
//              draws with OpenGL, and its plugin API names wxGLContext and
//              wxGLCanvas. This build draws charts with wxDC (ocpnUSE_GL is
//              not defined), so the two classes are only declared here.
// Copyright:   (c) 2026 AmiCradle Team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef _WX_GLCANVAS_OPENCPN_GL_OFF_H_
#define _WX_GLCANVAS_OPENCPN_GL_OFF_H_

#include "wx/defs.h"

class WXDLLIMPEXP_FWD_CORE wxGLContext;
class WXDLLIMPEXP_FWD_CORE wxGLCanvas;

#endif
