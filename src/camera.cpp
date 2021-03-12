#include "camera.h"

Free_camera::Free_camera(): Camera(), translation(glm::vec3(0.0f)), speed(2.0f)
{
	update();
}

Free_camera::Free_camera(const float aspect): Camera(aspect), translation(glm::vec3(0.0f)), speed(2.0f)
{
	update();
}

Free_camera::Free_camera(const float x, const float y, const float z): Camera(x, y, z), translation(glm::vec3(0.0f)), speed(2.0f)
{
	update();
}
Free_camera::Free_camera(const float x, const float y, const float z, const float aspect): Camera(x, y, z, aspect), translation(glm::vec3(0.0f)), speed(2.0f)
{
	update();
}
void Free_camera::update()
{
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
	position += translation;
	translation = glm::vec3(0.0f);
	look = glm::vec3(rotation * glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f ));
	up = glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	right = glm::cross(look, up);
	glm::vec3 point = position + look;
	view = glm::lookAt(position, point, up);
	set_projection_matrix();
}

void Free_camera::walk(const float dt)
{
	translation += dt * look;
	update();
}

void Free_camera::strafe(const float dt)
{
	translation += dt * right;
	update();
}

void Free_camera::lift(const float dt)
{
	translation += dt * up;
	update();
}

void Free_camera::set_translation_vector(const float x, const float y, const float z)
{
	translation = glm::vec3(x, y, z);
	update();
}

void Free_camera::rotate(const float y, const float p)
{
	yaw += y;
	pitch += p;
	if (pitch > 89.0f)
		pitch = 89.0f;
	else if (pitch < -89.0f)
		pitch = -89.0f;
	update();

}

glm::vec3 Free_camera::get_translation_vector() const
{
	return translation;
}

void Free_camera::set_speed(const float s)
{
	speed = s;
}

float Free_camera::get_speed() const
{
	return speed;
}


Camera::Camera(): view(glm::mat4(1.0f)), projection(glm::mat4(1.0f)), fov(45.0f), aspect_ratio(1.0f), Z_near(0.1f), Z_far(1000.f), yaw(0.0f), pitch(0.0f), position(glm::vec3(0.0f)), up(glm::vec3(0.0f, 0.0f, 1.0f)), look(glm::vec3(0.0f)), right(glm::vec3(0.0f))
{
}

Camera::Camera(const float aspect): Camera()
{
	aspect_ratio = aspect;
}

Camera::Camera(const float x, const float y, const float z): Camera()
{
	position = glm::vec3(x, y, z);
}

Camera::Camera(const float x, const float y, const float z, const float aspect): Camera(aspect)
{
	position = glm::vec3(x, y, z);
}

void Camera::set_projection_matrix()
{
	projection = glm::perspective(glm::radians(fov), aspect_ratio, Z_near, Z_far);
}

const glm::mat4 Camera::get_projection_matrix() const
{
	return projection;
}

const glm::mat4 Camera::get_view_matrix() const
{
	return view;
}

void Camera::set_position_vector(const float x, const float y, const float z)
{
	position = glm::vec3(x, y, z);
}

void Camera::set_fov(const float f)
{
	if (fov < 1.0f)
		fov = 1.0f;
	else if (fov > 180.0f)
		fov = 180.0f;
	else
	{
		fov += f;
		update();
	}
}

void Camera::set_aspect_ratio(const float a)
{
	aspect_ratio = a;
	update();
}

void Camera::set_rotation(const float y, const float p, const float r)
{
	yaw = y;
	pitch = p;
	roll = r;
}

const glm::vec3 Camera::get_position_vector() const
{
	return position;
}

const float Camera::get_fov() const
{
	return fov;
}
