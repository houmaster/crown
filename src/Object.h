#pragma once

#include "tweakable.h"

enum class ObjectType {
  none,
  player,
  pig_king,
  pig,
};

class Object {
public:
  Object(ObjectType type, float x, float y);

  explicit operator bool() const { return m_type != ObjectType::none; }

  void set_size(float width, float height);
  void update();
  void apply_force(float x, float y);

  float x() const { return m_x; }
  float y() const { return m_y; }
  float width() const { return m_width; }
  float height() const { return m_height; }
  float velocity_x() const { return m_velocity_x; }
  float velocity_y() const { return m_velocity_y; }
  float get_x_at(float frame_pos) const { return m_x + m_velocity_x * frame_pos; }
  float get_y_at(float frame_pos) const { return m_y + m_velocity_y * frame_pos; }
  bool on_ground() const { return m_on_ground; }

private:
  ObjectType m_type{ };
  float m_x{ };
  float m_y{ };
  float m_width{ };
  float m_height{ };
  float m_velocity_x{ };
  float m_velocity_y{ };
  bool m_on_ground{ };
};
