using Plots
using Plots.Measures
using Printf
function current(t, R, L, C, V0)
    α = R / (2L)
    ω0 = 1 / sqrt(L * C)
    
    if α > ω0
        
        # Overdamped
        
        β = sqrt(α^2 - ω0^2)
        return (V0 / (2L * β)) * exp(-α * t) * (exp(β * t) - exp(-β * t))
    elseif α == ω0

        # Critically damped
        
        return (V0 / L) * t * exp(-α * t)
    else
        
        # Underdamped (oscillatory)
        
        ω = sqrt(ω0^2 - α^2)
        return (V0 / (ω * L)) * exp(-α * t) * sin(ω * t)
    end
end

function magnetic_field_plot(dt, total_t)
    
    # Initialize
    
    μ0 = 4π * 1e-7
    v = 0 
    time = 0
    # Initial Position Parameter

    x = -16 / 100 # m

    # Pellet Characteristics
    
    mass_pellet = 0.89 / 1000 # kg
    pellet_radius = 3 / 1000 # m
    χ = 1000
    
    # Coil Parameters
    
    coil_radius = 4 / 1000 #m
    coil_length = 30 / 100 #m 
    n = 3500
    
    # Circuit Parameters
    L = μ0 * n^2 * π * coil_radius^2 / coil_length # H 
    C = 2e-3  # F
    R = 2 * sqrt(L/C) - 0.5  # Critical dampening
    V0 = 50 # V    

    # ----

    V_projectile = π * pellet_radius^3 * 4 / 3

    for t in 0:dt:total_t
        I = current(t, R, L, C, V0)
        term1 = (x - coil_length/2) / sqrt(coil_radius^2 + (x - coil_length/2)^2)
        term2 = (x + coil_length/2) / sqrt(coil_radius^2 + (x + coil_length/2)^2)
        d_term(pos) = coil_radius^2 / (coil_radius^2 + pos^2)^(1.5)
        grad = (μ0 * n * I / 2) * (d_term(x - coil_length/2) - d_term(x + coil_length/2))
        grad = -grad

        B = (μ0 * n * I / 2) * (term2 - term1)
        rho = 1.225
        Cd = 0.47
        Area = π * pellet_radius^2
        damp = 0.001 * v
        force = (V_projectile * χ/μ0) * B * grad
        a = (force - damp) / mass_pellet
        v += a * dt
        x += v * dt
        time += dt
        print("\rDirection: $(@sprintf("%.2f", x)), Speed: $(@sprintf("%.2f", v)), Time: $(@sprintf("%.2f", time))")
    end

    attr = (tickfontsize=6, guidefontsize=8, legendfontsize=6, margin=5mm)    

end

magnetic_field_plot(1e-6, 5)