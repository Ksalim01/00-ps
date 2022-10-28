#include <liburing.h>
#include <solution.h>
#include <stdio.h>
#include <stdlib.h>

typedef char byte;

const size_t BLOCK_SZ = (size_t)1 << 18;
const size_t QUEUE_READ_SZ = 4;
const size_t QUEUE_WRITE_SZ = 4;
size_t current_queue_size = 0;

struct io_data {
  int read_mode;
  int read_bytes;
  off_t offset;
  byte* buf;
};

struct io_data* new_io_data() {
  struct io_data* data = malloc(sizeof(struct io_data));
  data->buf = malloc(BLOCK_SZ);
  return data;
}

void free_io_data(struct io_data* data) {
  free(data->buf);
  free(data);
}

struct fd {
  int in;
  int out;
};

void push_task(struct fd* desc, struct io_uring* ring, struct io_data* data) {
  struct io_uring_sqe* sqe = io_uring_get_sqe(ring);

  if (data->read_mode) {
    io_uring_prep_read(sqe, desc->in, data->buf, BLOCK_SZ, data->offset);
  } else {
    io_uring_prep_write(sqe, desc->out, data->buf, data->read_bytes,
                        data->offset);
  }
  io_uring_sqe_set_data(sqe, data);
  io_uring_submit(ring);

  ++current_queue_size;
}

void push_read_task(struct fd* desc, struct io_uring* ring) {
  off_t offset = 0;

  for (size_t i = 0; i < QUEUE_READ_SZ; ++i) {
    struct io_data* data = new_io_data();
    data->offset = offset;
    data->read_mode = 1;
    push_task(desc, ring, data);
    offset += BLOCK_SZ;
  }
}

struct io_data* pop_task(struct io_uring* ring) {
  struct io_uring_cqe* cqe;
  io_uring_wait_cqe(ring, &cqe);
  struct io_data* data = io_uring_cqe_get_data(cqe);
  
  data->read_bytes = cqe->res;

  io_uring_cqe_seen(ring, cqe);

  --current_queue_size;
  return data;
}

int copy(int in, int out) {
  struct io_uring ring;
  io_uring_queue_init(QUEUE_READ_SZ + QUEUE_WRITE_SZ, &ring, 0);
  struct fd desc;
  desc.in = in;
  desc.out = out;

  push_read_task(&desc, &ring);
  
  int exit_code = 0;
  while (current_queue_size) {
    struct io_data* data = pop_task(&ring);
    if (data->read_mode && data->read_bytes > 0) {
      data->read_mode = 0;
      push_task(&desc, &ring, data);
    } else {
	  exit_code = data->read_bytes;
      free_io_data(data);
    }
  }

  return exit_code;
}
