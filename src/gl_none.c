/*
 * OpenCPN draws its charts with wxDC on AmigaOS 3 (OpenGL off), but a few
 * of its files still name GL calls on paths that only run with a GL chart
 * canvas: the S-52 library's texture fonts and symbol textures, MBTiles,
 * and the plugin overlay. There is never a GL canvas here, so these are
 * never reached; they are defined so that the program links without a GL
 * library. When the toolkit's wxGLCanvas comes, OpenGL goes on and this
 * file goes away.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <GL/gl.h>

void glBindTexture(GLenum target, GLuint texture) { (void)target; (void)texture; }
void glBlendFunc(GLenum s, GLenum d) { (void)s; (void)d; }
void glDeleteTextures(GLsizei n, const GLuint *t) { (void)n; (void)t; }
void glDisable(GLenum cap) { (void)cap; }
void glEnable(GLenum cap) { (void)cap; }
void glGenTextures(GLsizei n, GLuint *t) { GLsizei i; for (i = 0; i < n; i++) t[i] = 0; }
void glHint(GLenum target, GLenum mode) { (void)target; (void)mode; }
void glRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) { (void)a; (void)x; (void)y; (void)z; }
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) { (void)x; (void)y; (void)z; }
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width,
                  GLsizei height, GLint border, GLenum format, GLenum type,
                  const GLvoid *pixels) {
  (void)target; (void)level; (void)internalformat; (void)width; (void)height;
  (void)border; (void)format; (void)type; (void)pixels;
}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
  (void)target; (void)pname; (void)param;
}
