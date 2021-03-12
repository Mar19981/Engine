#ifndef CAMERA_H
#define CAMERA_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/euler_angles.hpp"
class Camera
{
protected:
	glm::mat4 view, projection;
	float fov, aspect_ratio, Z_near, Z_far, yaw, pitch, roll = 0;
	glm::vec3 look, up, right, position;

public:
	Camera();
	Camera(const float aspect);
	Camera(const float x, const float y, const float z);
	Camera(const float x, const float y, const float z, const float aspect);
	virtual ~Camera() = default;
	virtual void update() = 0;
	virtual void rotate(const float y, const float p) = 0;
	void set_projection_matrix();
	const glm::mat4 get_projection_matrix() const;
	const glm::mat4 get_view_matrix() const;
	void set_position_vector(const float x, const float y, const float z);
	void set_fov(const float f);
	void set_aspect_ratio(const float a);
	void set_rotation(const float y, const float p, const float r);
	const glm::vec3 get_position_vector() const;
	const float get_fov() const;

};
class Free_camera : public Camera
{
	glm::vec3 translation;
	float speed;
public:
	Free_camera();
	Free_camera(const float aspect);
	Free_camera(const float x, const float y, const float z);
	Free_camera(const float x, const float y, const float z, const float aspect);
	~Free_camera() = default;
	void update();
	void walk(const float dt);
	void strafe(const float dt);
	void lift(const float dt);
	void set_translation_vector(const float x, const float y, const float z);
	void rotate(const float y, const float p);
	glm::vec3 get_translation_vector() const;
	void set_speed(const float s);
	float get_speed() const;
};

#endif // !CAMERA_H
