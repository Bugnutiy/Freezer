#pragma once
/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR64(T, CODE)                      \
  {                                         \
    static uint64_t tmr##T = -T;            \
    if ((uint64_t)(millis() - tmr##T) >= T) \
    {                                       \
      tmr##T = millis();                    \
      CODE                                  \
    }                                       \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR32(T, CODE)                                \
  {                                                   \
    static uint32_t tmr##T = -T;                      \
    if ((uint32_t)((uint32_t)millis() - tmr##T) >= T) \
    {                                                 \
      tmr##T = millis();                              \
      CODE                                            \
    }                                                 \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR16(T, CODE)                                \
  {                                                   \
    static uint16_t tmr##T = -T;                      \
    if ((uint16_t)((uint16_t)millis() - tmr##T) >= T) \
    {                                                 \
      tmr##T = millis();                              \
      CODE                                            \
    }                                                 \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR8(T, CODE)                               \
  {                                                 \
    static uint8_t tmr##T = -T;                     \
    if ((uint8_t)((uint8_t)millis() - tmr##T) >= T) \
    {                                               \
      tmr##T = millis();                            \
      CODE                                          \
    }                                               \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR8_S(T, CODE)                             \
  {                                                 \
    static uint8_t tmr##T = -T;                     \
    if ((uint8_t)((uint8_t)millis() - tmr##T) >= T) \
    {                                               \
      tmr##T += T;                                  \
      CODE                                          \
    }                                               \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR16_S(T, CODE)                              \
  {                                                   \
    static uint16_t tmr##T = -T;                      \
    if ((uint16_t)((uint16_t)millis() - tmr##T) >= T) \
    {                                                 \
      tmr##T += T;                                    \
      CODE                                            \
    }                                                 \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR32_S(T, CODE)                              \
  {                                                   \
    static uint32_t tmr##T = -T;                      \
    if ((uint32_t)((uint32_t)millis() - tmr##T) >= T) \
    {                                                 \
      tmr##T += T;                                    \
      CODE                                            \
    }                                                 \
  }

/**
 * @brief Defines a macro for a timer that triggers after a specified time interval, with a specified code block to execute.
 * The timer uses the millis() function to measure the elapsed time.
 *
 * @param T The time interval in milliseconds.
 * @param CODE The code block to execute when the timer triggers.
 *
 * @return The macro does not return a value.
 */
#define TMR64_S(T, CODE)                    \
  {                                         \
    static uint64_t tmr##T = -T;            \
    if ((uint64_t)(millis() - tmr##T) >= T) \
    {                                       \
      tmr##T += T;                          \
      CODE                                  \
    }                                       \
  }
//----------------------------------------------------------------