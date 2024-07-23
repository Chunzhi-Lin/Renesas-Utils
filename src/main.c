#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <i2c.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

// #define ISL68224_SET_DMA_ADDR_CMD	0xc7
// #define ISL68224_GET_DMA_DATA_CMD	0xc5
// #define ISL68224_GET_DEVICE_ID_CMD	0xad
// #define ISL68224_GET_REVERSION_ID_CMD	0xae

// #define ISL68224_NVM_SLOT_NUM_REG	0xc2

// #define ISL68224_SET_DMA_ADDR_CMD	0xF7
// #define ISL68224_GET_DMA_DATA_CMD	0xF5
// #define ISL68224_GET_DEVICE_ID_CMD	0xAD
// #define ISL68224_GET_REVERSION_ID_CMD	0xAE

// #define ISL68224_NVM_SLOT_NUM_REG	0x1080

static struct
{
	int num;
	uint8_t set_dma_addr_cmd;
	uint8_t get_dma_data_cmd;
	uint8_t get_device_id_cmd;
	uint8_t get_reversion_id_cmd;
	uint8_t apply_settings_cmd;
	uint16_t nvm_slot_num_reg;
	uint16_t full_power_mode_enable;
	uint16_t voltage_regulation_enable;
	uint16_t check_program_result_reg;
	uint16_t bank_status_reg;
} device_info[] = {
	{0, 0xF7, 0xF5, 0xAD, 0xAE, 0xE7, 0x1080, 0x1095, 0x106d, 0x1400, 0x1401},
	{1, 0xC7, 0xC5, 0xAD, 0xAE,   -1,   0xC2,     -1,     -1,     -1,     -1},
};

static int device_id;

void dump(void *s, int len)
{
	int i;

	for (i = 0; i < len; ++i)
		printf("%02x ", ((uint8_t *)s)[i]);
	printf("\n");
}

static int i2c_bus_init(int bus, int addr)
{
	int fd;
	char dev_name[16];

	snprintf(dev_name, sizeof(dev_name), "/dev/i2c-%d", bus);
	fd = open(dev_name, O_RDONLY);
	if (fd < 0) {
		printf("open i2c device failed\n");
		return -1;
	}
	int err;
	err = ioctl(fd, I2C_SLAVE_FORCE, addr);
	if (err < 0) {
		printf("failed to set i2c slave address\n");
		close(fd);
		return -1;
	}

	return fd;
}

static void i2c_bus_destroy(int fd)
{
	close(fd);
}

int isl68224_set_dma_addr(int bus, uint16_t addr)
{
	uint8_t tmp[2];

	tmp[0] = addr & 0xff;
	tmp[1] = (addr >> 8) & 0xff;

	return i2c_smbus_write_i2c_block_data(bus, device_info[device_id].set_dma_addr_cmd, sizeof(tmp), tmp);
}

int isl68224_get_dma_data(int bus, uint16_t addr, unsigned int len, uint8_t *data)
{
	int err;

	err = isl68224_set_dma_addr(bus, addr);
	if (err) {
		fprintf(stderr, "failed to set dma address\n");
		return err;
	}
	return i2c_smbus_read_i2c_block_data(bus, device_info[device_id].get_dma_data_cmd, len, data);
}

unsigned int isl68224_get_nvm_slot_num(int bus)
{
	uint8_t tmp[4];

	isl68224_get_dma_data(bus, device_info[device_id].nvm_slot_num_reg, sizeof(tmp), tmp);

	return tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
}

unsigned int isl68224_get_device_id(int bus)
{
	uint8_t tmp[5];
	int err;

	err = i2c_smbus_read_i2c_block_data(bus, device_info[device_id].get_device_id_cmd, sizeof(tmp), tmp);

	printf("\rIC_DEVICE_ID =");
	dump(tmp, sizeof(tmp));

	return err;
}

unsigned int isl68224_get_reversion_id(int bus)
{
	uint8_t tmp[5];
	int err;

	err = i2c_smbus_read_i2c_block_data(bus, device_info[device_id].get_reversion_id_cmd, sizeof(tmp), tmp);

	printf("\rIC_DEVICE_REV =");
	dump(tmp, sizeof(tmp));

	return err;
}

unsigned int isl68224_enable_voltage_regulation(int bus)
{
	uint8_t tmp[4];
	uint8_t data[2];
	data[0] = 0x01;
	data[1] = 0x00;
	int err;

	isl68224_get_dma_data(bus, device_info[device_id].voltage_regulation_enable, sizeof(tmp), tmp);
	printf("\rvoltage regulation read=");
	dump(tmp, sizeof(tmp));
	tmp[0] |= (1 << 0);
	printf("\rvoltage regulation write=");
	dump(tmp, sizeof(tmp));
	err = i2c_smbus_write_i2c_block_data(bus, device_info[device_id].get_dma_data_cmd, sizeof(tmp), tmp);
	if (err){
		printf("err = %d\n", err);
	}else {
		err = i2c_smbus_write_i2c_block_data(bus, device_info[device_id].apply_settings_cmd, sizeof(data), data);
	}

	return err;

}

unsigned int isl68224_check_program_result(int bus)
{
	uint8_t tmp[4];
	int err;

	err = isl68224_get_dma_data(bus, device_info[device_id].check_program_result_reg, sizeof(tmp), tmp);
	if (err){
		printf("get program status failed\n");
		return 0;
	}
	printf("\rprogram status reg= ");
	dump(tmp, sizeof(tmp));

	return tmp[0];
}

unsigned int isl68224_get_bank_status_reg(int bus)
{
	uint8_t tmp[4];
	int err;

	err = isl68224_get_dma_data(bus, device_info[device_id].bank_status_reg,sizeof(tmp), tmp);
	if (err){
		printf("get bank status failed\n");
		return err;
	}
	printf("\rbank status reg = ");
	dump(tmp, sizeof(tmp));

	return 0;
}

static int hex2bin(char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	else if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;
	else if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	else
		return -EINVAL;
}

int isl68224_program(int bus, int addr, char *name)
{
	struct stat st;
	int err;
	char *hex;
	ssize_t size;

	err = stat(name, &st);
	if (err) {
		fprintf(stderr, "cannot get file status\n");
		return err;
	}

	hex = malloc(st.st_size + 1);

	if (!hex) {
		fprintf(stderr, "allocate buffer failed\n");
		return -ENOMEM;
	}

	int fd = open(name, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "cannot open file %s\n", name);
		return fd;
	}

	size = read(fd, hex, st.st_size);
	close(fd);
	if (size != st.st_size) {
		fprintf(stderr, "cannot get enough data\n");
		goto free_buf;
	}

	hex[st.st_size] = 0;

	/* printf("%s\n", hex); */

	int i, j, status_reg;
	int tag, nb, cmd;//, pa, crc;
	// int tag, nb, cmd, pa, crc;
	uint8_t line_buf[64];
	int line_size;
	uint8_t data_buf[32];
	uint8_t write_buf[32];
	int data_size;

	for (i = 0; i < st.st_size; ++i) {
		for (line_size = 0; i < st.st_size; ++i) {
			if (line_size > sizeof(line_buf)) {
				fprintf(stderr, "hex string too long\n");
				goto free_buf;
			}

			if (hex[i] == '\n')
				break;
			else if (isxdigit(hex[i]))
				line_buf[line_size++] = hex[i];
		}

		/* empty line */
		if (line_size == 0)
			continue;

		if (line_size % 2) {
			fprintf(stderr, "wrong hex number per line\n");
			goto free_buf;
		}

		/* convert hex string to binary */
		for (j = 0; j < line_size; j += 2) {
			data_buf[j / 2] = hex2bin(line_buf[j]) << 4 |
				hex2bin(line_buf[j + 1]);
		}
		data_size = line_size / 2;

		if (data_size < 2) {
			fprintf(stderr, "wrong format: line too short\n");
			goto free_buf;
		}

		tag = data_buf[0];
		nb = data_buf[1];

		if (nb + 2 != data_size) {
			fprintf(stderr, "wrong format: invalid byte count\n");
			goto free_buf;
		}

		if (tag != 0)
			/* skip lines that shouldnot burn into device */
			continue;

		if (nb < 3) {
			fprintf(stderr, "wrong format: invalid byte count\n");
			goto free_buf;
		}

		// pa = data_buf[2];
		cmd = data_buf[3];

		/* ignore, we donnot use PEC */
		// crc = data_buf[data_size - 1];

		// printf("%02X", tag);
		// printf("%02X", nb);
		// printf("%02X", pa);
		// printf("%02X", cmd);

		for (int k = 0; k < nb - 3; ++k){
			// printf("%02X", data_buf[4 + i]);
			write_buf[k] = data_buf[4 + k];
		}

		// printf("%02X", crc);
		// printf("\n");
		printf("  programming... %ld%%\r", (i + 1) * 100 / st.st_size);

		i2c_smbus_write_i2c_block_data(bus, cmd, nb - 3, write_buf);
	}

	printf("\n");

	sleep(2);

	status_reg = isl68224_check_program_result(bus);
	if (status_reg & (1 << 0))
		printf("Program seccess\n");
	else
		printf("Program failed! status reg= 0x%x\n", status_reg);

	isl68224_get_bank_status_reg(bus);


free_buf:
	free(hex);

	return 0;
}

static const char * const script_usage_info =
"./script i2c-bus slave-addr device {filename}\n"
"	i2c-bus:	0: CPU0 I2C0  4: CPU1 I2C0\n"
"	device:		0: ISL68127   1: ISL68224\n";

int main(int argc, char *argv[])
{
	int bus;
	int addr;

	if (argc < 4 || argc > 5) {
		printf("%s\n", script_usage_info);
		return -1;
	}

	bus = atoi(argv[1]);
	if (bus != 0 && bus != 4) {
		printf("%s\n", script_usage_info);
		return -1;
	}

	addr = atoi(argv[2]);

	device_id = atoi(argv[3]);
	if (device_id != 0 && device_id != 1) {
		printf("%s\n", script_usage_info);
		return -1;
	}

	bus = i2c_bus_init(bus, addr);
	if (bus < 0)
		return -1;

	printf("availiable nvm slots: %u\n", isl68224_get_nvm_slot_num(bus));

	isl68224_get_device_id(bus);
	isl68224_get_reversion_id(bus);

	if (argc == 5) {
		if (device_id == 0)
			isl68224_enable_voltage_regulation(bus);

		sleep(1);

		isl68224_program(bus, addr, argv[4]);
	}

	i2c_bus_destroy(bus);

	return 0;
}
