#include "Camera.hpp"

Camera::Camera(glm::dmat4x4 Projection, glm::dmat4x4 View)
{
	m_Projection = Projection;
	m_View = View;
	m_Rotation = {m_View};
	// no way i know that we can get the Up, Eye and Center vectors from this
}

Camera::Camera(const Camera &Copy)
{
	m_Projection = Copy.m_Projection;
	m_View = Copy.m_View;
	m_Position = Copy.m_Position;
	m_LookingAt = Copy.m_LookingAt;
	m_FPSUp = Copy.m_FPSUp;
	m_LookingAtPosition = Copy.m_LookingAtPosition;
	m_FPSmode = Copy.m_FPSmode;
	m_Rotation = Copy.m_Rotation;
}

Camera::Camera(Camera &&Move)
{
	m_Projection = std::move(Move.m_Projection);
	m_View = std::move(Move.m_View);
	m_Position = std::move(Move.m_Position);
	m_LookingAt = std::move(Move.m_LookingAt);
	m_FPSUp = std::move(Move.m_FPSUp);
	m_LookingAtPosition = Move.m_LookingAtPosition;
	m_FPSmode = Move.m_FPSmode;
	m_Rotation = std::move(Move.m_Rotation);
}

const Camera &Camera::operator=(const Camera &Copy)
{
	m_Projection = Copy.m_Projection;
	m_View = Copy.m_View;
	m_Position = Copy.m_Position;
	m_LookingAt = Copy.m_LookingAt;
	m_FPSUp = Copy.m_FPSUp;
	m_LookingAtPosition = Copy.m_LookingAtPosition;
	m_FPSmode = Copy.m_FPSmode;
	m_Rotation = Copy.m_Rotation;

	return *this;
}

const Camera &Camera::operator=(Camera &&Move)
{
	m_Projection = std::move(Move.m_Projection);
	m_View = std::move(Move.m_View);
	m_Position = std::move(Move.m_Position);
	m_LookingAt = std::move(Move.m_LookingAt);
	m_FPSUp = std::move(Move.m_FPSUp);
	m_LookingAtPosition = Move.m_LookingAtPosition;
	m_FPSmode = Move.m_FPSmode;
	m_Rotation = std::move(Move.m_Rotation);

	return *this;
}

void Camera::SetProjection(const glm::dmat4x4 &Projection)
{
	m_Projection = Projection;
}
glm::dmat4x4 Camera::GetProjection() const { return m_Projection; }

void Camera::SetView(const glm::dmat4x4 &View)
{
	m_View = View;
	m_Rotation = {View};
}
glm::dmat4x4 Camera::GetView() const { return m_View; }
glm::dvec3 Camera::GetPosition() const { return m_Position; }

glm::dmat4x4 Camera::GetMVP() const { return m_Projection * m_View; }

glm::dvec3 Camera::GetViewVector() const
{
	return m_Rotation * glm::dvec3{0, 0, 1};
}
glm::dvec3 Camera::GetUpVector() const
{
	return m_Rotation * glm::dvec3{0, 1, 0};
}
glm::dvec3 Camera::GetLeftVector() const
{
	return m_Rotation * glm::dvec3{1, 0, 0};
}

void Camera::CreateProjection(
    double fovY,
    double AspectRatio,
    double NearClip,
    double FarClip)
{
	m_Projection
	    = glm::perspective<double>(fovY, AspectRatio, NearClip, FarClip);
}
void Camera::CreateProjectionX(
    double fovX,
    double AspectRatio,
    double NearClip,
    double FarClip)
{
	double fovY = (2 * glm::atan(glm::tan(fovX * 0.5) * AspectRatio));
	m_Projection
	    = glm::perspective<double>(fovY, AspectRatio, NearClip, FarClip);
}

void Camera::LookIn(glm::dvec3 Direction, glm::dvec3 Up /*={0, 1, 0}*/)
{
	m_Rotation = glm::quatLookAt(glm::normalize(Direction), glm::normalize(Up));
	if (m_FPSmode)
	{
		m_FPSUp = Up;
	}
	m_LookingAtPosition = false;
	UpdateView();
}
void Camera::LookIn(
    bool FPSmode,
    glm::dvec3 Direction,
    glm::dvec3 Up /*={0, 1, 0}*/)
{
	m_FPSmode = FPSmode;
	LookIn(Direction, Up);
}

void Camera::LookAt(
    bool KeepLookingAt,
    glm::dvec3 Position,
    glm::dvec3 Up /*={0, 1, 0}*/)
{
	m_LookingAtPosition = KeepLookingAt;
	if (m_LookingAtPosition)
	{
		m_LookingAt = Position;
	}
	m_Rotation = glm::quatLookAt(
	    glm::normalize(m_Position - Position),
	    glm::normalize(Up));
	UpdateView();
}

void Camera::LookAt(
    bool KeepLookingAt,
    bool FPSmode,
    glm::dvec3 Position,
    glm::dvec3 Up /*={0, 1, 0}*/)
{
	m_FPSmode = FPSmode;
	LookAt(KeepLookingAt, Position, Up);
}

void Camera::Move(glm::dvec3 DeltaPosition)
{
	m_Position += DeltaPosition;
	UpdateView();
}
void Camera::MoveTo(glm::dvec3 Position)
{
	m_Position = Position;
	UpdateView();
}

void Camera::UpdateView()
{
	m_Rotation = glm::normalize(m_Rotation);
	if (m_LookingAtPosition)
	{
		auto Up = m_FPSmode ? m_FPSUp
		                    : glm::inverse(m_Rotation) * glm::dvec3{0, 1, 0};
		m_Rotation = glm::quatLookAt(
		    glm::normalize(m_Position - m_LookingAt),
		    glm::normalize(Up));
	}
	else if (m_FPSmode)
	{
		auto Forward = m_Rotation * glm::dvec3{0, 0, 1};
		m_Rotation
		    = glm::quatLookAt(glm::normalize(Forward), glm::normalize(m_FPSUp));
	}
	m_View = glm::translate(glm::dmat4{1}, m_Position)
	         * static_cast<glm::dmat4>(m_Rotation);
}

void Camera::RotateBy(glm::dquat Rot)
{
	m_Rotation *= Rot;
	UpdateView();
}
