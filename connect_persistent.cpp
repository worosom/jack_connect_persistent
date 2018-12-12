/** @file persistent_client.c
 *
 * @brief A Jack-client that will watch for port registration changes and ensure that input and output-port stay connected if they are available.
 */
extern "C" {
#include <jack/jack.h>
#include <jack/types.h>
}
#include <iostream>           // std::cout
#include <thread>             // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <chrono>

const char *client_name = "persistent";
const char *server_name = NULL;
const char **input_ports;
const char **output_ports;
jack_options_t options = JackNullOption;
jack_status_t status;
jack_client_t *client;

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

typedef void (*JackPortRegistrationCallback)(jack_port_id_t port, int /* register */, void* arg);
typedef void (*JackPortConnectCallback)(jack_port_id_t a, jack_port_id_t b, int connect, void *arg);

void clientNotify() {
  if (!ready) {
    std::unique_lock<std::mutex> lck (mtx);
    ready = true;
    cv.notify_all();
  }
}

  void
port_registration_cb (jack_port_id_t port, int register, void* arg)
{
  clientNotify();
}

  void
port_connect_cb (jack_port_id_t a, jack_port_id_t b, int connect, void *arg)
{
  clientNotify();
}

  void
update_ports(char *argv[]) {
  output_ports = jack_get_ports(client, argv[1], NULL, JackPortIsOutput);
  input_ports = jack_get_ports(client, argv[2], NULL, JackPortIsInput);
}

	void
connect()
{
  for (unsigned int i = 0; i < sizeof(output_ports); i++) {
    if (output_ports == NULL || output_ports[i] == NULL)
      return;
    unsigned int j = i % sizeof(input_ports);
    if (input_ports == NULL || input_ports[j] == NULL)
      return;
    jack_port_t *out_port = jack_port_by_name(client, output_ports[i]);
      if (jack_connect(client, output_ports[i], input_ports[j]) == 0) {
        std::cout << "(re)connected " << output_ports[i] << " ➔ " << input_ports[j] << std::endl;
      }
      std::cout << "(re)connected " << output_ports[i] << " ➔ " << input_ports[j] << std::endl;
  }
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
  void
jack_shutdown (void *arg)
{
  fprintf(stderr, "JACK shut down, exiting ...\n");
  exit(1);
}

void
init ()
{
  /* open a client connection to the JACK server */

  client = jack_client_open(client_name, options, &status, server_name);
  if (client == NULL) {
    fprintf(stderr, "jack_client_open() failed, "
        "status = 0x%2.0x\n", status);
    if (status & JackServerFailed) {
      fprintf(stderr, "Unable to connect to JACK server\n");
    }
    exit (1);
  }
  if (status & JackServerStarted) {
    fprintf(stderr, "JACK server started\n");
  }
  if (status & JackNameNotUnique) {
    client_name = jack_get_client_name (client);
    fprintf(stderr, "unique name `%s' assigned\n", client_name);
  }

  jack_set_port_registration_callback(client, port_registration_cb, 0);
  jack_set_port_connect_callback(client, port_connect_cb, 0);

  /* tell the JACK server to call `jack_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
     */

  jack_on_shutdown (client, jack_shutdown, 0);

  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */

  if (jack_activate (client)) {
    fprintf(stderr, "cannot activate client");
    exit(1);
  }
  std::cout << "client activated" << std::endl;
}

  int
main (int argc, char **argv)
{
	init();

  /* keep running until stopped by the user */

  std::unique_lock<std::mutex> lck(mtx);
	while (true) {
		update_ports(argv);
		connect();
    jack_free(output_ports);
    jack_free(input_ports);
		ready = false;
		while (!ready) {
      /* wait for notification thread */
			cv.wait(lck);
		}
	}

  /* this is never reached but if the program
     had some other way to exit besides being killed,
     they would be important to call.
     */

  jack_client_close (client);
  exit (0);
}

