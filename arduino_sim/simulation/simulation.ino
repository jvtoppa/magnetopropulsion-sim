using real = float;

constexpr real mi_0 = 4 * PI * 1e-7;

struct PelletCharacteristics
{
  real mass = 0.89 / 100;
  real radius = 3 / 1000;
  real chi = 1000;
  real volume = PI * pow(radius, 3) * 4 / 3;
};

struct Coil
{
  real radius = 4 / 1000;
  real length = 30 / 100;
  real n = 3500;
};

class Circuit
{
    private:
        Coil c;
        real L; 
        real C = 2e-3;
        real R = 0.5;
        real V0 = 50;
        
        void set_L(Coil c)
        {
          L = mi_0 * pow(c.n, 2) * PI * pow(c.radius, 2) / c.length;
        }
    
    public:
        Circuit(const Coil& coil = Coil{}, real capacitance = 2e-3, real resistance = 0.5, real voltage = 50)
        : c(coil), C(capacitance), R(resistance), V0(voltage)
        {
            set_L(coil);
        }
        
        void set_R(real r)
        {
          R = r;
        }

        void set_L(real l)
        {
          L = l;
        }

        void set_C(real c)
        {
          C = c;
        }

        void set_V0(real v0)
        {
          V0 = v0;
        }

        real get_R() const { return R; }
        real get_L() const { return L; }
        real get_C() const { return C; }
        real get_V0() const { return V0; }
};


class Pellet
{
  public:
  
    PelletCharacteristics characteristics;
  
  private:

    real x = 0; //Distance
    real v = 0; //Velocity
    real a = 0; //Acceleration
    real force = 0; //Force

  public:
    
    Pellet(const PelletCharacteristics& chars = PelletCharacteristics{}, real x_pos = 0, real vel = 0, real acc = 0, real force = 0) 
    : characteristics(chars), x(x_pos), v(vel), a(acc), force(force) {}

    real get_x() const { return x; }
    real get_a() const { return a; }
    real get_v() const { return v; }
    real get_F() const { return force; }
    void set_x(real x_new) { x = x_new; }
      
    void update(real dt, real f_new)
    {
        force = f_new;
        a = force / characteristics.mass;
        v += a * dt;
        x += v * dt;
    }
    
};


class Experiment
{
    private: 
        Pellet pellet;
        Coil coil;
        Circuit circuit;
        real t = 0;
        
        real d_term(real pos, real coil_radius) const
        {
            return pow(coil_radius, 2) / pow(pow(coil_radius, 2) + pow(pos, 2), 1.5);
        }

        real current(real time)
        {
          real v0 = circuit.get_V0();
          real r = circuit.get_R();
          real l = circuit.get_L();
          real c = circuit.get_C();
          
          real alpha = r / (2 * l);
          real omega_0 = 1 / sqrt(l * c);
          real beta = 0;
          real error = 16e-8;
          
          if (alpha > omega_0)
          {
            beta = sqrt(alpha * alpha - omega_0 * omega_0);
            return (v0 / (2 * l * beta)) * exp(-alpha * time) * exp(beta * time);
          }

          if (abs(alpha - omega_0) <= error)
          {
            return (v0 / l) * time * exp(-alpha * time);
          }

          real omega = sqrt(-alpha * alpha + omega_0 * omega_0);
          return (v0 / (omega * l)) * exp(-alpha * time) * sin(omega * time);
        }



    public: 
        Experiment(const Pellet& p = Pellet{}, const Coil& c = Coil{}, const Circuit& ci = Circuit{}) 
        : pellet(p), coil(c), circuit(ci) {};


        void step(real dt)
        {
            t += dt;
                      
            real I = current(t);
      
            real term1 = ((pellet.get_x() - coil.length) / 2) / sqrt(pow(coil.radius, 2) + pow((pellet.get_x() - coil.length) / 2, 2));
            real term2 = ((pellet.get_x() + coil.length) / 2) / sqrt(pow(coil.radius, 2) + pow((pellet.get_x() + coil.length) / 2, 2));
            
            real B = (mi_0 * coil.n * I / 2) * (term2 - term1);
            
            real grad = (mi_0 * coil.n * I / 2) * (d_term(term1, coil.radius) - d_term(term2, coil.radius));
            grad = -grad;
            
            real F_magnetic = (pellet.characteristics.volume * pellet.characteristics.chi / mi_0) * B * grad;
            
            real damping_coefficient = 0.001;
            real F_damping = damping_coefficient * pellet.get_v();
            
            real F_total = F_magnetic - F_damping;
            
            pellet.update(dt, F_total);
        }
        
        // Getters for results
        real get_time() const { return t; }
        const Pellet& get_pellet() const { return pellet; }
        const Circuit& get_circuit() const { return circuit; }
        
        // Get pellet state
        real get_pellet_x() const { return pellet.get_x(); }
        real get_pellet_v() const { return pellet.get_v(); }
        real get_pellet_a() const { return pellet.get_a(); }
        real get_pellet_F() const { return pellet.get_F(); }
        
        real get_current()
        {
            return current(t);
        }
        
        bool is_pellet_in_coil() const
        {
            real x = pellet.get_x();

            return (pellet.get_x() >= -coil.length/2 && x <= coil.length/2);
        }
        
        real get_muzzle_velocity() const
        {
            return pellet.get_v();
        }
};


unsigned long last_time = 0;
real dt = 0.0001;
bool simulation_running = true;
Experiment Exp;    
void setup()
{
    Serial.begin(9600);
    
    PelletCharacteristics myChars;
    myChars.mass = 0.0089;      
    myChars.radius = 0.003;     
    myChars.chi = 1000;                
    
    Coil myCoil;
    myCoil.radius = 0.004;        
    myCoil.length = 0.3;          
    myCoil.n = 3500;                   
    
    Circuit myCircuit(myCoil, 0.002, 0.5, 50);  
    
    Pellet myPellet(myChars);
    
    Exp = Experiment(myPellet, myCoil, myCircuit);
    
    last_time = micros();
    
    Serial.println("Simulation Started!");
    Serial.println("Time (s)\tPosition (m)\tVelocity (m/s)\tCurrent (A)\tIn Coil");
}

void loop()
{
    unsigned long current_time = micros();
    unsigned long elapsed = current_time - last_time;
    
    if (elapsed >= (dt * 1000000))
    {
        if (simulation_running)
        {
            Exp.step(dt);
            
            static int step_counter = 0;
            step_counter++;
            
            if (step_counter >= 1000)
            {
                step_counter = 0;
                
                Serial.print(Exp.get_time(), 6);
                Serial.print("\t");
                Serial.print(Exp.get_pellet_x(), 6);
                Serial.print("\t");
                Serial.print(Exp.get_pellet_v(), 6);
                Serial.print("\t");
                Serial.print(Exp.get_current(), 6);
                Serial.print("\t");
                Serial.println(Exp.is_pellet_in_coil());
            }
            
            if (!Exp.is_pellet_in_coil() && Exp.get_time() > 0.01)
            {
                simulation_running = false;
                Serial.println("\n--- Simulation Complete ---");
                Serial.print("Muzzle Velocity: ");
                Serial.print(Exp.get_muzzle_velocity(), 6);
                Serial.println(" m/s");
                Serial.print("Final Position: ");
                Serial.print(Exp.get_pellet_x(), 6);
                Serial.println(" m");
                Serial.print("Simulation Time: ");
                Serial.print(Exp.get_time(), 6);
                Serial.println(" s");
            }
            
            if (Exp.get_time() > 0.5)
            {
                simulation_running = false;
                Serial.println("\n--- Time Limit Reached --");
            }
        }
        
        last_time = current_time;
    }
}