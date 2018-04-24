#include "worker.h"

const static char* TAG = "WORKER";

sniffer_context sniffers[64];
discovered_ble_devices_list ble_devices_list;
victims_to_track_list victims_list;

/*
 * Add or update the specified ble device mac address
 * Caution: Must lock mutex first before calling
 */
void add_or_update_discovered_sniffers(uint8_t* mac, discovered_sniffers node) {
	uint32_t i, j;

	uint8_t found_mac=0, found_sniffer=0;
	// Check if the MAC exists in list
	for(i=0; i<ble_devices_list.list_size; i++) {
		if(mac_address_match(ble_devices_list.devices_list[i].ble_addr, mac) == 1) {
			found_mac=1;
			break;
		}
	}

	if(found_mac == 1) { // This MAC address is already in the list, we update the list by either add new discovered_sniffers or update existing discovered_sniffers with same ID
		for(j=0; j<ble_devices_list.devices_list[i].discovered_sniffer_nodes_count; j++) {
			if(ble_devices_list.devices_list[i].sniffer_nodes[j].sniffer_id == node.sniffer_id) {
				found_sniffer=1;
				break;
			}
		}

		// MAC Address Exists
		// Sniffer Exists in this MAC Address
		if(found_sniffer == 1) {
			ble_devices_list.devices_list[i].sniffer_nodes[j].rssi=node.rssi;
		}else{ // MAC Address Exists, Sniffer does not exists in this MAC Address
			ble_devices_list.devices_list[i].sniffer_nodes[ble_devices_list.devices_list[i].discovered_sniffer_nodes_count].sniffer_id=node.sniffer_id;
			ble_devices_list.devices_list[i].sniffer_nodes[ble_devices_list.devices_list[i].discovered_sniffer_nodes_count].rssi=node.rssi;
			ble_devices_list.devices_list[i].discovered_sniffer_nodes_count=ble_devices_list.devices_list[i].discovered_sniffer_nodes_count+1;
		}
	}else{ // MAC Address does not exists, simply add this mac address and add this sniffer into this address
		// Check for list size
		if(ble_devices_list.list_size == ble_devices_list.list_max_size) {
			// Enlarge list
			ble_devices_list.list_max_size = ble_devices_list.list_max_size*2;
			ble_devices_list.devices_list = realloc(ble_devices_list.devices_list, (sizeof(discovered_ble_device) * ble_devices_list.list_max_size));
			if(ble_devices_list.devices_list == NULL) {
				log_error(TAG, "Unable to allocate memory for BLE devices list");
				exit(-1);
			}
		}

		// Add MAC to list
		MAC_ADDR_ASSIGN(ble_devices_list.devices_list[ble_devices_list.list_size].ble_addr, mac)


		// Add discovered_sniffers to list
		ble_devices_list.devices_list[ble_devices_list.list_size].sniffer_nodes[0].sniffer_id=node.sniffer_id;
		ble_devices_list.devices_list[ble_devices_list.list_size].sniffer_nodes[0].rssi=node.rssi;
		ble_devices_list.devices_list[ble_devices_list.list_size].discovered_sniffer_nodes_count=1;

		// Check if the MAC is being tracked (aka a victim)
		char buf[64];
		uint8_t result=0;
		database_check_victim_active(getMACString(ble_devices_list.devices_list[ble_devices_list.list_size].ble_addr, buf, 64), &result);
		if(result == 1) {
			ble_devices_list.devices_list[ble_devices_list.list_size].is_tracking=1;
		}

		ble_devices_list.list_size=ble_devices_list.list_size+1;
	}

}

/*
 * Using dumb method to rebuild the BLE Devices list
 * 0. Mutex Lock
 * 1. Delete everything inside the list
 * 2. Regenerage the list
 * 3. Done
 * 4. Mutex unlock
 */
void rebuild_ble_devices_list() {
	uint32_t i, j;

	// Mutex lock
	pthread_mutex_lock(&ble_devices_list.mutex);

	// Delete all entry
	ble_devices_list.list_size=0;
	ble_devices_list.list_max_size=128;
	ble_devices_list.devices_list = realloc(ble_devices_list.devices_list, (sizeof(discovered_ble_device) * ble_devices_list.list_max_size));
	if(ble_devices_list.devices_list == NULL) {
		log_error(TAG, "Unable to reallocate memory for BLE devices list");
		exit(-1);
	}
	for(i=0; i<ble_devices_list.list_max_size; i++) {
		ble_devices_list.devices_list[i].discovered_sniffer_nodes_count=0;
		ble_devices_list.devices_list[i].is_tracking=0;
	}

	// Regenerate BLE Devices List from sniffer context
	for(i=0; i<64; i++) {
		if(sniffers[i].connected) {
			pthread_mutex_lock(&sniffers[i].mutex);
			for(j=0; j<sniffers[i].ble_devices_count; j++) {

				// See if this BLE device is exists in the list
				// If yes, add a new discovered_sniffers entry;
				// Otherwise, push a new BLE device address to the list then add a new discovered_sniffers entry to this address
				discovered_sniffers node;
				node.sniffer_id=sniffers[i].info.sniffer_id;
				node.rssi=sniffers[i].ble_devices[j].rssi;
				add_or_update_discovered_sniffers(sniffers[i].ble_devices[j].ble_mac_address, node);
			}
			pthread_mutex_unlock(&sniffers[i].mutex);
		}
	}

	// Mutex unlock
	pthread_mutex_unlock(&ble_devices_list.mutex);
}

/*
 * Using dumb method to rebuild the Victims (to track) list
 * 0. Mutex Lock
 * 1. Delete everything inside the list
 * 2. Regenerage the list
 * 3. Done
 * 4. Mutex unlock
 */
void rebuild_victims_list() {
	// Mutex lock
	pthread_mutex_lock(&victims_list.mutex);

	// Delete all entry
	victims_list.list_size=0;
	victims_list.list_max_size=128;
	victims_list.victims_list = realloc(victims_list.victims_list, (sizeof(tracked_victim) * victims_list.list_max_size));
	if(victims_list.victims_list == NULL) {
		log_error(TAG, "Unable to reallocate memory for victims list");
		exit(-1);
	}

	// Regenerate victims list from SQLite3 Database
	if(database_get_all_tracked_victims(&victims_list) == 1) {
		log_error(TAG, "SQLite3 execution failed.");
		exit(-1);
	}

	// Mutex unlock
	pthread_mutex_unlock(&victims_list.mutex);
}

void *worker_thread(void *arg) {
	log_info(TAG, "Worker thread is starting...");

	//char buf[64];
	while(1) {
		//uint32_t i, j;

		rebuild_ble_devices_list();
		rebuild_victims_list();


		//log_success(TAG, "===INFO===");

		/*for(i=0; i<64; i++) {
			if(sniffers[i].connected) {
				pthread_mutex_lock(&sniffers[i].mutex);
				log_info(TAG, "ID: %d, IP: %s, BATT: %d, CHG: %d, DEV:%d",
						sniffers[i].info.sniffer_id,
						getIPString(sniffers[i].info.ip_address, buf, 64),
						getSnifferBatteryPercentage(sniffers[i].info),
						isSnifferCharging(sniffers[i].info),
						sniffers[i].ble_devices_count);
				for(j=0; j<sniffers[i].ble_devices_count; j++) {
					log_info(TAG, "BLE: %s RSSI: %d",
							getMACString(sniffers[i].ble_devices[j].ble_mac_address, buf, 64),
							sniffers[i].ble_devices[j].rssi);
				}
				pthread_mutex_unlock(&sniffers[i].mutex);
			}
		}*/

		/*log_success(TAG, "===INFO===\nDevices: %d", ble_devices_list.list_size);
		pthread_mutex_lock(&ble_devices_list.mutex);
		for(i=0; i<ble_devices_list.list_size; i++) {
			log_error(TAG, "MAC: %s", getMACString(ble_devices_list.devices_list[i].ble_addr, buf, 64));
			for(j=0; j<ble_devices_list.devices_list[i].discovered_sniffer_nodes_count; j++) {
				log_info(TAG, "\t ID: %d RSSI: %d", ble_devices_list.devices_list[i].sniffer_nodes[j].sniffer_id, ble_devices_list.devices_list[i].sniffer_nodes[j].rssi);
			}
		}
		pthread_mutex_unlock(&ble_devices_list.mutex);*/


		nanosleep((const struct timespec[]){{0, (1000UL*1000UL*TICK_MS)}}, NULL);
	}
}
