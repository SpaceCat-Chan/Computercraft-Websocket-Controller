#pragma once

#include <iostream>
#include <tuple>

#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

constexpr double NaN = std::numeric_limits<double>::signaling_NaN();

/**
 * \class Camera
 * \brief a class that controls the projection and view matrix with a simpler interface
 * 
 * 
 */
class Camera
{
	glm::dmat4x4 m_Projection, m_View;
	glm::dvec3 m_Position, m_LookVector, m_Up={0,1,0};
	double m_Pitch = 0, m_Yaw = glm::radians(-90.0), m_Roll = 0;

	/**
	 * \brief updates the View matrix to reflect new variables
	 */
	void UpdateView();

	enum class Locked
	{
		Position,
		Direction,
		PitchYaw
	};
	Locked m_LookAt = Locked::Direction;

public:
	Camera() = default;
	/**
	 * \brief Construct a Camera with a projection matrix and a view matrix
	 * 
	 * \param Projection the projection matrix to use
	 * \param View the view matrix to use
	 */
	Camera(glm::dmat4x4 Projection, glm::dmat4x4 View);
	Camera(const Camera &Copy);
	Camera(Camera &&Move);

	const Camera &operator=(const Camera &Copy);
	const Camera &operator=(Camera &&Move);

	void SetProjection(glm::dmat4x4 Projection);
	glm::dmat4x4 GetProjection() const;

	/**
	 * \brief return the vector signifying the direction the camera is looking
	 * 
	 * the vector is either the direction or location, depending on the value of ViewLock
	 */
	glm::dvec3 GetViewVector() const;
	/**
	 * \brief return what the value of the ViewVector means
	 */
	Locked GetViewLock();

	void SetView(glm::dmat4x4 View);
	glm::dmat4x4 GetView() const;

	/**
	 * \brief get the product of the projection and view matrix
	 */
	glm::dmat4x4 GetMVP() const;

	/**
	 * \brief sets LookAt to Locked::Position, meaning the camera will try to look at the same position
	 */
	void LockViewPosition();
	/**
	 * \brief set LookAt to Locked::Direction, meaning the camera will try to look in the same direction
	 */
	void LockViewDirection();
	/**
	 * \brief set LookAt to Locked::PitchYaw, meaning the camera willl try to look  in the same direction, but with pitch and yaw
	 */
	void LockViewPitchYaw();

	/**
	 * \brief Creates a projection matrix
	 * 
	 * \param fovY the fov in the y direction
	 * \param AspectRatio the aspect ratio of the window
	 * \param NearClip the near clipping plane
	 * \param FarClip the far clipping plane
	 */
	void CreateProjection(double fovY, double AspectRatio, double NearClip, double FarClip);
	/**
	 * \brief Creates a projection matrix
	 * 
	 * \param fovX the fov in the x direction
	 * \param AspectRatio the aspect ratio of the window
	 * \param NearClip the near clipping plane
	 * \param FarClip the far clipping plane
	 */
	void CreateProjectionX(double fovX, double AspectRatio, double NearClip, double FarClip);

	/**
	 * \brief will make the camera look in a direction
	 * 
	 * will also set LookAt to to Locked::Direction, meaning if the camera moves
	 * it will keep loking in the same direction
	 * 
	 * \param Direction the direction to look in
	 * \param Up a vector pointing upwards
	 */
	void LookIn(glm::dvec3 Direction, glm::dvec3 Up = {0, 1, 0});
	/**
	 * \brief will make the camera look in a direction
	 * 
	 * will also set LookAt to Locked::PitchYaw, meaning that direction is locked and rotation is pitch and yaw based
	 * instead of vector based
	 * 
	 * \param Pitch the pitch of the camera
	 * \param Yaw the yaw of the camera
	 * \param Roll the roll of the camera
	 */
	void LookIn(double Pitch = 0, double Yaw = glm::radians(-90.0), double Roll = 0);
	/**
	 * \brief will make the camera look at a position
	 * 
	 * will also set LookAt to be Locked::Position, meaning if the camera moves
	 * it will keep loking in the same direction
	 * 
	 * \param Position the position to look at
	 * \param Up a vector pointing upwards
	 */
	void LookAt(glm::dvec3 Position, glm::dvec3 Up = {0, 1, 0});

	/**
	 * \brief offsets Pitch, Yaw and Roll by a certain amount
	 * 
	 * \param Pitch the pitch to offset by
	 * \param Yaw the yaw to offset by
	 * \param Roll the roll to offset by
	 * \param MinPitch the minimum allowed pitch value
	 * \param MaxPitch the maximum allowed pitch value
	 * \param MinYaw the minimum allowed yaw value
	 * \param MaxYaw the maximum allowed yaw value
	 * \param MinRoll the minimum allowed roll value
	 * \param MaxRoll the maximum allowed roll value
	 * \param RespectRoll if Roll should be acounted for when changing Pitch and Yaw
	 * 
	 * Pitch will be clamped between MinPitch and MaxPitch
	 * 
	 * Yaw will be clamped between MinYaw and MaxYaw
	 * 
	 * Roll will be clamped between MinRoll and MaxRoll
	 * 
	 * MinPitch > MaxPitch is undefined behavior
	 * 
	 * MinYaw > MaxYaw is undefined behavior
	 * 
	 * MinRoll > MaxRoll is undefined behavior
	 * 
	 * NaN is seen as a null value (a value to be ignored)
	 */
	void OffsetPitchYaw(double Pitch,
						double Yaw,
						double Roll,
						double MinPitch = NaN,
						double MaxPitch = NaN,
						double MinYaw = NaN,
						double MaxYaw = NaN,
						double MinRoll = NaN,
						double MaxRoll = NaN,
						bool RespectRoll = true);

	glm::dvec3 GetPosition() const;
	/**
	 * \brief makes the camera move by a certain amount
	 * 
	 * \param DeltaPosition the amount to move by
	 */
	void Move(glm::dvec3 DeltaPosition);
	/**
	 * \brief makes the camera move to a certain position
	 * 
	 * \param Position the position to move to
	 */
	void MoveTo(glm::dvec3 Position);
};
