if "%GLFW_INCS%"=="" set GLFW_INCS=C:\GL\GLFW\include
@rem NOTE: use forward slashes in GLFW_LIBS here (this is a mabu bug)
if "%GLFW_LIBS%"=="" set GLFW_LIBS=C:/GL/GLFW/lib-vc2015
if "%GLAD_ROOT%"=="" set GLAD_ROOT=C:\GL\GLAD_ROOT

mabu simple_gl_app.package "GLFW_INCS=%GLFW_INCS%" "GLFW_LIBS=%GLFW_LIBS%" "GLAD_ROOT=%GLAD_ROOT%"

