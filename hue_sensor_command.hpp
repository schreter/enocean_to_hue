/*!
 * @file
 * @brief Command handler to post commands to Hue bridge via virtual sensor.
 */
#pragma once

#include <cstdint>

/*!
 * @brief Command handler to post commands to Hue bridge via virtual sensor.
 *
 * Create a sensor by posting to /api/&lt;api_key&gt;/sensors:
 * <pre>
 * {
 *   "state": {
 *     "status": 0
 *   },
 *   "config": {
 *     "on": true
 *   },
 *   "name": "ExternalInput",
 *   "type": "CLIPGenericStatus",
 *   "modelid": "GenericCLIP",
 *   "manufacturername": "Philips",
 *   "swversion": "1.0",
 *   "uniqueid": "external_input",
 *   "recycle": false
 * }
 * </pre>
 *
 * Use created sensor ID as constructor argument. Posting to the command
 * handler, you can change value of the sensor. Rules can then react on
 * the change of the sensor value.
 */
class hue_sensor_command
{
public:
  /*!
   * @brief Construct command hander to send commands to the Hue bridge via a sensor.
   *
   * @param ip IP address of the bride.
   * @param api_key API key assigned by the bridge.
   * @param sensor_id sensor ID assigned by the bridge.
   *
   * The command handler PUTs to the Sensor URL in form
   * <tt>http://&lt;ip&gt;/api/&lt;api_key&gt;/sensors/&lt;sensor_id&gt;</tt>.
   * It PUTs JSON document in the form:
   * <tt>{"state":{"status": &lt;value&gt;}}</tt>
   */
  explicit hue_sensor_command(uint32_t ip, const char* api_key, int sensor_id) noexcept :
    ip_(ip), api_key_(api_key), sensor_id_(sensor_id)
  {}

  ~hue_sensor_command() noexcept {}

  /*!
   * @brief Post a value to the sensor.
   *
   * The queue with values is not arbitrarily long. Only recent values
   * will be actually posted, too old values will be lost, if not confirmed
   * by the bridge in time.
   *
   * @param value value to post.
   */
  void post(int32_t value);

  /// Get FD to poll on, if any.
  int get_fd() const noexcept { return fd_; }

  /// Get events to poll for.
  short get_events() const noexcept;

  /// Process events on file descriptor.
  void poll();

private:
  /// Current state of the handler.
  enum class state
  {
    idle,           ///< No connection and queue empty.
    connecting,     ///< Connecting to the remote side.
    sending,        ///< Sending data to the remote side.
    receiving,      ///< Receiving reply from the remote side.
    unknown         ///< Unknown state.
  };

  /// Element of the queue.
  struct queue_element
  {
    int32_t value;      ///< Value to post.
    int64_t timestamp;  ///< Milliseconds since some common point in time.
  };

  /// Get current timestamp.
  static int64_t timestamp() noexcept;

  /// Prepare buffer with first command in queue.
  bool prepare_buffer(int64_t timestamp) noexcept;

  /// Start connecting to the remote side.
  void start_connect();

  /// Maximum 4 commands in the queue.
  static inline constexpr auto MAX_QUEUE_SIZE = 4;
  /// Maximum 0.5 seconds for the bridge to accept the command.
  static inline constexpr auto MAX_EVENT_AGE = 500000;

  /// Remote IP address.
  const uint32_t ip_;
  /// API key.
  const char* const api_key_;
  /// Sensor ID to post to.
  const int sensor_id_;

  /// File descriptor of the current connection, if any.
  int fd_ = -1;
  /// Current state of the connection.
  state state_ = state::idle;
  /// Current queue size.
  uint8_t queue_size_ = 0;
  /// Queue with commands to send.
  queue_element queue_[MAX_QUEUE_SIZE];

  /// Send pointer.
  const uint8_t* send_ptr_ = nullptr;
  /// Outstanding send size.
  uint16_t send_outstanding_size_ = 0;
  /// Send buffer with current command.
  char buffer_[512];
};
