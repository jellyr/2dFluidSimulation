#pragma once
#include <limits>
#include <vector>

#include "core.h"
#include "vec.h"

#include "MarkerParticles.h"
#include "ScalarGrid.h"
#include "VectorGrid.h"
#include "LevelSet2d.h"
#include "Transform.h"

#include "Integrator.h"

#include "AdvectField.h"
#include "ExtrapolateField.h"
#include "MarkerParticlesSimulation.h"

///////////////////////////////////
//
// MarkerParticlesSimulation.h/cpp
// Ryan Goldade 2016
//
// Wrapper class around the staggered MAC grid fluid simulator 
// (which stores face-aligned velocities and pressure).
// Handles velocity, surface, viscosity field advection,
// pressure projection, viscosity and velocity extrapolation.
//
////////////////////////////////////

class FlipParticlesSimulation
{
public:
	FlipParticlesSimulation(const Transform& xform, Vec2st nx, size_t nb = 5)
		: m_xform(xform)
		, m_moving_solids(false)
		, m_solve_viscosity(false)
		, m_enforce_bubbles(false)
		, m_volume_correction(false)
		, m_air_volume(false)
		, m_st_scale(0.)
	{
		m_vel = VectorGrid<Real>(m_xform, nx, VectorGridSettings::STAGGERED);
		m_collision_vel = VectorGrid<Real>(m_xform, nx, 0., VectorGridSettings::STAGGERED);

		m_surface = LevelSet2D(m_xform, nx, nb);
		m_collision = LevelSet2D(m_xform, nx, nb);

		m_particles = MarkerParticles(m_surface.dx() / 2., 4, 2., true);

		m_air_surface = LevelSet2D(m_xform, nx, nb);
		m_air_particles = MarkerParticles(m_surface.dx() / 2., 4, 2.);
	}

	void set_collision_volume(const LevelSet2D& collision);
	void set_collision_velocity(const VectorGrid<Real>& collision_vel);
	void set_surface_volume(const LevelSet2D& surface);
	void set_surface_velocity(const VectorGrid<Real>& vel);
	void set_surface_tension(Real st_scale)
	{
		m_st_scale = st_scale;
	}

	void set_enforce_bubbles()
	{
		m_enforce_bubbles = true;
	}

	void set_volume_correction()
	{
		m_volume_correction = true;
		m_target_volume = compute_volume(true);
		m_accum_error = 0.;
	}

	void set_viscosity(const ScalarGrid<Real>& visc_coeff)
	{
		assert(m_surface.is_matched(visc_coeff));
		m_variableviscosity = visc_coeff;
		m_solve_viscosity = true;
	}

	void set_viscosity(Real visc_coeff = 1.)
	{
		m_variableviscosity = ScalarGrid<Real>(m_surface.xform(), m_surface.size(), visc_coeff);
		m_solve_viscosity = true;
	}

	void set_air_volume();

	void disable_moving_solids() { m_moving_solids = false; }

	void add_surface_volume(const LevelSet2D& surface);

	template<typename ForceSampler>
	void add_force(const ForceSampler& force, Real dt);

	void add_force(const Vec2R& force, Real dt);

	void advect_surface(Real dt, IntegratorSettings::Integrator order);
	void advect_viscosity(Real dt, IntegratorSettings::Integrator order);
	void advect_velocity(Real dt, IntegratorSettings::Integrator order);

	// Super sample voxels to compute the volume of the liquid or air
	Real compute_volume(bool liquid = true) const;

	// Perform pressure project, viscosity solver, extrapolation, surface and velocity advection
	void run_simulation(Real dt, Renderer& renderer);

	// Useful for CFL
	Real max_vel_mag() { return m_vel.max_magnitude(); }

	// Rendering tools
	void draw_grid(Renderer& renderer) const;
	void draw_surface(Renderer& renderer);
	void draw_air(Renderer& renderer);
	void draw_collision(Renderer& renderer);
	void draw_collision_vel(Renderer& renderer, Real length) const;
	void draw_velocity(Renderer& renderer, Real length, bool from_particles = false) const;

private:

	// Simulation containers
	VectorGrid<Real> m_vel, m_collision_vel;
	LevelSet2D m_surface, m_collision;
	ScalarGrid<Real> m_variableviscosity;

	// Marker particle to track the liquid
	MarkerParticles m_particles;

	// Volume correction
	bool m_air_volume;
	LevelSet2D m_air_surface;
	MarkerParticles m_air_particles;

	Transform m_xform;

	bool m_moving_solids, m_solve_viscosity, m_enforce_bubbles, m_volume_correction;
	Real m_st_scale, m_target_volume;
	Real m_accum_error;
};