#include "driver.h"

#include <math.h>

// CONSTRUCTORS
driver::driver()
{
    // Set default values
    driver::m_current_position = 0;
    driver::m_prior_state = 0;
    driver::m_pulses_missed = 0;

    // Set the parameters.
    driver::m_cpr = 400;

    // Set up the qmatrix, which is to be indexed by (prior_state, current_state)
    driver::m_transition_matrix = {{0, 1, -1, 2}, {1, 0, 2, -1}, {-1, 2, 0, 1}, {2, 1, -1, 0}};
}
driver::~driver()
{
    // Do nothing, virtual destructor.
}

// METHODS
void driver::initialize(uint32_t gpio_pin_a, uint32_t gpio_pin_b, uint32_t ppr)
{
    // Store the cpr and spin_ratios.
    driver::m_cpr = ppr * 2UL;  // Two counts for each pulse (rising/falling edge).

    // Call the extended class's initialize_gpio method.
    initialize_gpio(gpio_pin_a, gpio_pin_b);

    // Initialize the state using the extended class's read methods.
    bool level_a = read_gpio(gpio_pin_a);
    bool level_b = read_gpio(gpio_pin_b);
    driver::m_mutex.lock();
    driver::initialize_state(level_a, level_b);
    driver::m_mutex.unlock();
}
void driver::set_home()
{
    driver::m_mutex.lock();
    // Set current position to zero.
    driver::m_current_position = 0;
    // Reset the missed pulses flag.
    driver::m_pulses_missed = 0;
    driver::m_mutex.unlock();
}
double driver::get_position(bool reset)
{
    // Get current position in CPR.
    driver::m_mutex.lock();
    double position = static_cast<double>(driver::m_current_position);
    if(reset)
    {
        driver::m_current_position = 0;
        driver::m_pulses_missed = 0;
    }
    driver::m_mutex.unlock();

    // Return position in radians.
    return position / static_cast<double>(driver::m_cpr) * 2.0 * M_PIf64;
}

void driver::tick_a(bool level)
{
    driver::m_mutex.lock();
    // Calculate the new state.
    uint32_t new_state = (driver::m_prior_state & 1) | (static_cast<uint32_t>(level) << 1);
    // Update the encoder's state with the new state.
    driver::update_state(new_state);
    driver::m_mutex.unlock();
}
void driver::tick_b(bool level)
{
    driver::m_mutex.lock();
    // Calculate the new state.
    uint32_t new_state = (driver::m_prior_state & 2) | static_cast<uint32_t>(level);
    // Update the encoder's state with the new state.
    driver::update_state(new_state);
    driver::m_mutex.unlock();
}
void driver::initialize_state(bool level_a, bool level_b)
{
    // Initialize the prior state with the current state.
    driver::m_prior_state = (static_cast<uint32_t>(level_a) << 1) | static_cast<uint32_t>(level_b);
}
void driver::update_state(uint32_t new_state)
{
    // Get the transition value.
    int transition = driver::m_transition_matrix.at(driver::m_prior_state).at(new_state);

    // Check if there was a valid transition.
    if(transition == 2)
    {
        driver::m_pulses_missed += 1;
    }
    else
    {
        // Update the position.
        driver::m_current_position += transition;
    }

    // Update the prior state.
    driver::m_prior_state = new_state;
}

// PROPERTIES
uint64_t driver::pulses_missed()
{
    return driver::m_pulses_missed;
}