#include "Application.h"
//#include "vld.h"
int main(int argc, char** argv)
{
	try
	{
		Engine app("Franciszek Ksawery Drudzki-Lubecki", 800, 600);
		app.create_camera(1.0f, 0.5f, -1.0f);
		//app.toogle_wireframe();
		app.translate_camera(0, 1.0f, 2.0f, -0.5f);
		app.create_box(2.0f, 3.0f, 0.5f);
		app.change_texture(2, R"(src\tex\tex2.jpg)");
		app.create_model(R"(src\models\chalet.obj)", R"(src\tex\chalet.jpg)", 3.0f, 0.0f, -1.0f);
		app.create_camera();
		app.rotate_model(3, 10.0f, 20.0f, 40.0f);
		app.set_camera_position(2, 1.0f, 1.0f, 1.0f);
		app.translate_model(0, 1.0f, 1.0f, 1.0f);
		app.change_fov(0, 90.0f);
		app.create_plane(5.0f, 5.0f, 0.0f, -4.0f, 0.0f);
		app.create_sphere(5.0f, 13.0f, -4.0f, 0.0f);
		app.change_texture(5, R"(src\tex\tex1.jpg)");
		app.create_model(R"(src\models\teapot.obj)", -10.0f, 3.2f, 5.0f);
		app.change_texture(6, R"(src\tex\chalet.jpg)");
		app.change_texture(4, R"(src\tex\tex1.png)");
		app.run();
	}
	catch (const std::exception& err)
	{
		std::cout << err.what();
	}

	return 0;
}