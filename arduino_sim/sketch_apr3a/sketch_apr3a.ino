  using real = double;

  constexpr real mi_0 = 4 * PI * 1e-7;
  constexpr real error = 1;


  struct PelletCharacteristics
  {
    real mass = 0.89 / 100;
    real radius = 3.0 / 1000.0;
    real chi = 1000;
    real volume = PI * pow(radius, 3) * 4 / 3;
  };

  struct Coil
  {
    real radius = 4.0 / 1000.0;
    real length = 0.075;
    real n = 13500;
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
      
      void reset(real x_pos = 0, real vel = 0, real acc = 0, real force_val = 0)
      {
          x = x_pos;
          v = vel;
          a = acc;
          force = force_val;
      }
  };


  class Experiment
  {
      public:
      
          Coil coil;
          bool simulation_running = true;
          real simulation_time = 0;
          unsigned long last_real_time = 0;
      private: 
          Pellet pellet;
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
                return (v0 / (2 * l * beta)) * exp(-alpha * time) * (exp(beta * time) - exp(-beta * time));
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
              
              real x = pellet.get_x();
              real L = coil.length;
              real R = coil.radius;
              
              real z_plus = x + L/2;  
              real z_minus = x - L/2;  

              real term1 = z_plus / sqrt(R*R + z_plus*z_plus);
              real term2 = z_minus / sqrt(R*R + z_minus*z_minus);
              real B = (mi_0 * coil.n * I / 2) * (term1 - term2);
              
              real d_term1 = (R*R) / pow(R*R + z_plus*z_plus, 1.5);
              real d_term2 = (R*R) / pow(R*R + z_minus*z_minus, 1.5);
              real grad = (mi_0 * coil.n * I / 2) * (d_term1 - d_term2);
              
              real F_magnetic = (pellet.characteristics.volume*pellet.characteristics.chi / mi_0) * B * grad;
              
              real damping_coefficient = 0.001;
              real F_damping = damping_coefficient * pellet.get_v();
              real F_total = F_magnetic - F_damping;

              pellet.update(dt, F_total);
          }
          
          real get_time() const { return t; }
          const Pellet& get_pellet() const { return pellet; }
          const Circuit& get_circuit() const { return circuit; }
          
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
              return (pellet.get_x() >= -coil.length/2 && pellet.get_x() <= coil.length/2);
          }
          
          void restart_sim()
          {
              t = 0;
              simulation_time = 0;
              simulation_running = true;
              last_real_time = millis();
              
              real length_over_two = coil.length / 2.0;
              real start_x = -(length_over_two + length_over_two * 0.01);
              pellet.reset(start_x, 0, 0, 0);
          }

  };


Experiment Exp;
const real TARGET_DT = 0.01;
bool last_button_state = HIGH;
unsigned long last_debounce_time = 0;
const unsigned long debounce_delay = 50;

void setup()
{
    Serial.begin(115200);  
    delay(100);  
    pinMode(8, OUTPUT);  
    pinMode(2, INPUT_PULLUP);
    PelletCharacteristics chars;
    
    Coil coil;
    Circuit circ(coil, 0.002, 0.5, 50);
    Pellet pellet(chars);
    
    real length_over_two = coil.length / 2.0;
    real start_x = -(length_over_two + length_over_two * 0.01);
    pellet.set_x(start_x);
    
    Exp = Experiment(pellet, coil, circ);
    
    Exp.last_real_time = micros();
    Serial.println("---\n");
    Serial.println("Start!");
    Serial.println("Time(s)\tPosition(m)\tVelocity(m/s)\tCurrent(A)\tIn Coil");
    Serial.println("");
    digitalWrite(8, HIGH);
}

  void loop()
  {
    bool button_reading = digitalRead(2);
    
    if (button_reading != last_button_state) {
        last_debounce_time = millis();
    }
    
    if ((millis() - last_debounce_time) > debounce_delay) {
        if (button_reading == LOW && Exp.simulation_running == false) {
            Serial.println("\n--- RESTARTING SIMULATION ---\n");
            Exp.restart_sim();
            digitalWrite(8, HIGH);
        }
    }

    last_button_state = button_reading;

      unsigned long current_real_time = micros();
      unsigned long elapsed_real = current_real_time - Exp.last_real_time;
      
      if (Exp.simulation_running)
      {        
          unsigned long target_dt_us = TARGET_DT * 1000000.0;

          unsigned long steps_to_run = elapsed_real / target_dt_us;
          
          for (unsigned long i = 0; i < steps_to_run && Exp.simulation_running; i++)
          {
              Exp.step(TARGET_DT);
              Exp.simulation_time += TARGET_DT;
              
              if (abs(Exp.get_current()) <= error)
              {

                  Serial.print(Exp.get_time(), 6);
                  Serial.print("\t");
                  Serial.print(Exp.get_pellet_x(), 6);
                  Serial.print("\t");
                  Serial.print(Exp.get_pellet_v(), 6);
                  Serial.print("\t");
                  Serial.print(Exp.get_current(), 3);
                  Serial.print("\t");
                  Serial.println(Exp.is_pellet_in_coil());
                  
                  Serial.print("Projectile velocity: ");
                  Serial.print(Exp.get_pellet_v(), 6);
                  Serial.println(" m/s");
                  Exp.simulation_running = false;
                  break;

              }
              
              if (Exp.simulation_time > 1.0)
              {
                  Exp.simulation_running = false;
                  Serial.println("\n- Error - ");
                  break;
              }
          }
          
          Exp.last_real_time += steps_to_run * target_dt_us;
      }   

      else
      {
        digitalWrite(8, LOW);
      }


  }