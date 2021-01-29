#include "Camera.hpp"

#include <tuple>

Camera::Camera(glm::dmat4x4 Projection, glm::dmat4x4 View)
{
	m_Projection = Projection;
	m_View = View;
	// no way i know that we can get the Up, Eye and Center vectors from this
}

Camera::Camera(const Camera &Copy)
{
	m_Projection = Copy.m_Projection;
	m_View = Copy.m_View;
	m_Position = Copy.m_Position;
	m_LookVector = Copy.m_LookVector;
	m_Up = Copy.m_Up;
	m_LookAt = Copy.m_LookAt;
}

Camera::Camera(Camera &&Move)
{
	m_Projection = std::move(Move.m_Projection);
	m_View = std::move(Move.m_View);
	m_Position = std::move(Move.m_Position);
	m_LookVector = std::move(Move.m_LookVector);
	m_Up = std::move(Move.m_Up);
	m_LookAt = Move.m_LookAt;
}

const Camera &Camera::operator=(const Camera &Copy)
{
	m_Projection = Copy.m_Projection;
	m_View = Copy.m_View;
	m_Position = Copy.m_Position;
	m_LookVector = Copy.m_LookVector;
	m_Up = Copy.m_Up;
	m_LookAt = Copy.m_LookAt;

	return *this;
}

const Camera &Camera::operator=(Camera &&Move)
{
	m_Projection = std::move(Move.m_Projection);
	m_View = std::move(Move.m_View);
	m_Position = std::move(Move.m_Position);
	m_LookVector = std::move(Move.m_LookVector);
	m_Up = std::move(Move.m_Up);
	m_LookAt = Move.m_LookAt;

	return *this;
}

void Camera::SetProjection(glm::dmat4x4 Projection)
{
	m_Projection = Projection;
}
glm::dmat4x4 Camera::GetProjection() const { return m_Projection; }

void Camera::SetView(glm::dmat4x4 View) { m_View = View; }
glm::dmat4x4 Camera::GetView() const { return m_View; }
glm::dvec3 Camera::GetPosition() const { return m_Position; }

glm::dmat4x4 Camera::GetMVP() const { return m_Projection * m_View; }

void Camera::LockViewPosition()
{
	m_LookAt = Locked::Position;
	UpdateView();
}
void Camera::LockViewDirection()
{
	m_LookAt = Locked::Direction;
	UpdateView();
}
void Camera::LockViewPitchYaw()
{
	m_LookAt = Locked::PitchYaw;
	UpdateView();
}

glm::dvec3 Camera::GetViewVector() const { return m_LookVector; }
Camera::Locked Camera::GetViewLock() { return m_LookAt; }

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
	m_LookVector = glm::normalize(Direction);
	m_Up = glm::normalize(Up);

	LockViewDirection();
}

void Camera::LookIn(
    double Pitch /*=0*/,
    double Yaw /*=glm::radians(-90)*/,
    double Roll /*=0*/)
{
	m_Pitch = Pitch;
	m_Yaw = Yaw;
	m_Roll = Roll;

	LockViewPitchYaw();
}

void Camera::LookAt(glm::dvec3 Position, glm::dvec3 Up /*={0, 1, 0}*/)
{
	m_LookVector = Position;
	m_Up = glm::normalize(Up);

	LockViewPosition();
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

std::tuple<glm::dvec3, glm::dvec3>
GetViewAndUp(double Pitch, double Yaw, double Roll, glm::dvec3 NoRollUP)
{
	glm::dvec3 LookResult = glm::normalize(glm::dvec3{
	    glm::cos(Yaw) * glm::cos(Pitch),
	    glm::sin(Pitch),
	    glm::sin(Yaw) * glm::cos(Pitch)});

	glm::dvec3 Right = glm::cross(LookResult, glm::normalize(NoRollUP));
	glm::dvec3 TrueUp = glm::cross(LookResult, Right) * -1.0;
	TrueUp = glm::rotate(glm::dmat4x4(1), Roll, LookResult)
	         * glm::dvec4(TrueUp, 1);
	return {LookResult, TrueUp};
}

std::tuple<double, double> GetPitchYawFromLookVector(glm::dvec3 LookVector)
{
	double Pitch = glm::asin(LookVector.y);
	double Yaw;
	if (glm::cos(Pitch) == 0)
	{
		Yaw = 0;
	}
	else
	{
		Yaw = glm::asin(LookVector.z / glm::cos(Pitch));
	}

	return {Pitch, Yaw};
}

void Camera::UpdateView()
{
	if (m_LookAt == Locked::Direction)
	{
		m_View = glm::lookAt(m_Position, m_Position + m_LookVector, m_Up);
	}
	else if (m_LookAt == Locked::Position)
	{
		m_View = glm::lookAt(m_Position, m_LookVector, m_Up);
	}
	else
	{
		glm::dvec3 TrueUp;
		std::tie(m_LookVector, TrueUp)
		    = GetViewAndUp(m_Pitch, m_Yaw, m_Roll, m_Up);
		m_View = glm::lookAt(m_Position, m_Position + m_LookVector, TrueUp);
	}
}

void Camera::OffsetPitchYaw(
    double Pitch,
    double Yaw,
    double Roll,
    double MinPitch /* = NaN*/,
    double MaxPitch /* = NaN*/,
    double MinYaw /* = NaN*/,
    double MaxYaw /* = NaN*/,
    double MinRoll /* = NaN*/,
    double MaxRoll /* = NaN*/,
    bool RespectRoll /* = true*/)
{
	m_Roll += Roll;
	if (std::isnan(MinRoll) == false)
	{
		if (std::isnan(MaxRoll) == false)
		{
			m_Roll = glm::clamp(m_Roll, MinRoll, MaxRoll);
		}
		else
		{
			m_Roll = glm::max(m_Roll, MinRoll);
		}
	}
	else
	{
		if (std::isnan(MaxRoll) == false)
		{
			m_Roll = glm::min(m_Roll, MaxRoll);
		}
	}

	if (RespectRoll)
	{
		auto [LookResult, _] = GetViewAndUp(Pitch, Yaw, 0, m_Up);
		LookResult = glm::vec3(
		    glm::rotate(glm::dmat4(1), m_Roll, glm::dvec3{1, 0, 0})
		    * glm::dvec4(LookResult, 0));
		std::tie(Pitch, Yaw) = GetPitchYawFromLookVector(LookResult);
	}

	m_Pitch += Pitch;
	if (std::isnan(MinPitch) == false)
	{
		if (std::isnan(MaxPitch) == false)
		{
			m_Pitch = glm::clamp(m_Pitch, MinPitch, MaxPitch);
		}
		else
		{
			m_Pitch = glm::max(m_Pitch, MinPitch);
		}
	}
	else
	{
		if (std::isnan(MaxPitch) == false)
		{
			m_Pitch = glm::min(m_Pitch, MaxPitch);
		}
	}

	m_Yaw += Yaw;
	if (std::isnan(MinYaw) == false)
	{
		if (std::isnan(MaxYaw) == false)
		{
			m_Yaw = glm::clamp(m_Yaw, MinYaw, MaxYaw);
		}
		else
		{
			m_Yaw = glm::max(m_Yaw, MinYaw);
		}
	}
	else
	{
		if (std::isnan(MaxYaw) == false)
		{
			m_Yaw = glm::min(m_Yaw, MaxYaw);
		}
	}

	UpdateView();
}

std::tuple<glm::dmat4, glm::dmat4>
Camera::RotateVector(double x, double y, bool position_fix) const noexcept
{
	glm::dvec3 lookDirection = m_LookVector;
	if (m_LookAt == Locked::Position)
	{
		lookDirection = glm::normalize(lookDirection - m_Position);
	}
	auto rotation_x = glm::rotate(glm::dmat4{1}, x, m_Up);
	auto rotation_y
	    = glm::rotate(glm::dmat4{1}, y, glm::cross(lookDirection, m_Up));
	if (m_LookAt == Locked::Position && position_fix)
	{
		rotation_x = glm::translate(glm::dmat4{1}, m_Position) * rotation_x
		             * glm::translate(glm::dmat4{1}, -m_Position);
		rotation_y = glm::translate(glm::dmat4{1}, m_Position) * rotation_y
		             * glm::translate(glm::dmat4{1}, -m_Position);
	}
	return {rotation_x, rotation_y};
}

void Camera::Rotate(double x, double y, bool retain_up /*=true*/) noexcept
{
	auto [rotation_x, rotation_y] = RotateVector(x, y, true);
	glm::dvec3 forwards_x = rotation_x * glm::dvec4(m_LookVector, 1);
	glm::dvec3 full_forwards = rotation_y * glm::dvec4(forwards_x, 1);
	if (retain_up)
	{
		if (glm::abs(glm::dot(full_forwards, m_Up))
		    < glm::cos(glm::radians(1.0)))
		{
			m_LookVector = full_forwards;
		}
		else
		{
			m_LookVector = forwards_x;
		}
	}
	else
	{
		m_LookVector = full_forwards;
		m_Up = rotation_y * (rotation_x * glm::dvec4{m_Up, 0});
	}
	UpdateView();
}

void Camera::RotateAround(
    double x,
    double y,
    std::optional<glm::dvec3> position,
    bool retain_up)
{
	if (m_LookAt != Locked::Position && !position)
	{
		return;
	}
	auto around = position.value_or(m_LookVector);
	auto a_to_p = m_Position - around;
	auto [rotation_x, rotation_y] = RotateVector(x, y, false);
	a_to_p = rotation_x * glm::dvec4{a_to_p, 0};
	glm::dvec3 final_a_to_p = rotation_y * glm::dvec4{a_to_p, 0};
	if (m_LookAt != Locked::Position || !retain_up
	    || glm::abs(glm::dot(
	           glm::normalize(final_a_to_p + around - m_LookVector),
	           m_Up))
	           < glm::cos(glm::radians(1.0)))
	{
		m_Position = final_a_to_p + around;
	}
	else
	{
		m_Position = a_to_p + around;
	}
	if (!retain_up && m_LookAt == Locked::Position)
	{
		m_Up = rotation_y * rotation_x * glm::dvec4{m_Up, 0};
	}
	UpdateView();
}
