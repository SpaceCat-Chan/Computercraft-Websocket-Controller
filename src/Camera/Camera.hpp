#pragma once

#include <iostream>
#include <tuple>

#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

constexpr double NaN = std::numeric_limits<double>::signaling_NaN();

/**
 * \class Camera
 * \brief a class that controls the projection and view matrix with a simpler
 * interface
 *
 *
 */
class Camera
{
	glm::dmat4x4 m_Projection, m_View;
	glm::dvec3 m_Position, m_LookingAt, m_FPSUp = {0, 1, 0};
	glm::dquat m_Rotation{0.0, glm::dvec3{1, 0, 0}};

	/**
	 * \brief updates the View matrix to reflect new variables
	 */
	void UpdateView();

	bool m_LookingAtPosition = false;
	bool m_FPSmode = true;

	public:
	Camera() = default;
	Camera(glm::dmat4x4 Projection, glm::dmat4x4 View);
	Camera(const Camera &Copy);
	Camera(Camera &&Move);

	const Camera &operator=(const Camera &Copy);
	const Camera &operator=(Camera &&Move);

	void SetProjection(const glm::dmat4x4 &Projection);
	glm::dmat4x4 GetProjection() const;

	glm::dvec3 GetViewVector() const;
	glm::dvec3 GetUpVector() const;
	glm::dvec3 GetLeftVector() const;

	void SetView(const glm::dmat4x4 &View);
	glm::dmat4x4 GetView() const;

	glm::dmat4x4 GetMVP() const;

	void CreateProjection(
	    double fovY,
	    double AspectRatio,
	    double NearClip,
	    double FarClip);
	void CreateProjectionX(
	    double fovX,
	    double AspectRatio,
	    double NearClip,
	    double FarClip);

	void LookIn(glm::dvec3 Direction, glm::dvec3 Up = {0, 1, 0});
	void LookIn(bool FPSmode, glm::dvec3 Direction, glm::dvec3 Up = {0, 1, 0});

	void
	LookAt(bool KeepLookingAt, glm::dvec3 Position, glm::dvec3 Up = {0, 1, 0});
	void LookAt(
	    bool KeepLookingAt,
	    bool FPSmode,
	    glm::dvec3 Position,
	    glm::dvec3 Up = {0, 1, 0});

	void RotateBy(glm::dquat Rot);

	glm::dvec3 GetPosition() const;
	void Move(glm::dvec3 DeltaPosition);
	void MoveTo(glm::dvec3 Position);
};
