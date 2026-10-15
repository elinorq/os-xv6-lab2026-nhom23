#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define PGSIZE   4096
#define MAX_PAGES 32     // tổng số trang tối đa sẽ mở rộng tới
#define MAX_TRIES 200     // số lần thử lại nếu chưa tìm thấy (cấp phát mới hoàn toàn)

static int
is_alnum(char c)
{
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z');
}

// Kiểm tra chuỗi bắt đầu tại mem[start], không vượt quá mem[limit-1].
// Trả về độ dài chuỗi (không tính '\0') nếu hợp lệ, -1 nếu không hợp lệ
// hoặc không tìm thấy '\0' trong phạm vi còn lại.
static int
valid_candidate_len(char *mem, int start, int limit)
{
  int len = 0;
  int i = start;

  while (i < limit && mem[i] != '\0') {
    i++;
    len++;
  }
  if (i >= limit)          // không gặp '\0' trong vùng cấp phát -> không an toàn
    return -1;
  if (len == 0)             // chuỗi rỗng -> loại
    return -1;
  for (int k = 0; k < len; k++) {
    if (!is_alnum(mem[start + k]))
      return -1;             // có ký tự không phải chữ/số -> loại
  }
  return len;
}

// Quét đoạn [from, to) của mem (to <= tổng kích thước đã cấp phát thật sự),
// in TẤT CẢ ứng viên hợp lệ tìm được. msg chỉ dùng để định vị, KHÔNG in ra.
static int
scan_range(char *mem, int from, int to, char *msg, int msg_len)
{
  int found = 0;

  for (int i = from; i <= to - msg_len; i++) {
    int match = 1;
    for (int j = 0; j < msg_len; j++) {
      if (mem[i + j] != msg[j]) {
        match = 0;
        break;
      }
    }
    if (!match)
      continue;

    int start = i + msg_len;
    if (start >= to)
      continue;

    int len = valid_candidate_len(mem, start, to);
    if (len < 0)
      continue;

    // In đúng <secret>, không kèm "Here it is: ", bằng write() với độ dài đã xác định.
    write(1, mem + start, len);
    write(1, "\n", 1);
    found++;
  }
  return found;
}

int
main(int argc, char *argv[])
{
  char *msg = "Here it is: ";
  int msg_len = strlen(msg);
  int total_found = 0;

  for (int attempt = 0; attempt < MAX_TRIES && total_found == 0; attempt++) {
    // Mỗi lần thử: bắt đầu từ một heap "sạch" (sbrk hiện tại), rồi MỞ RỘNG
    // dần bằng sbrk(PGSIZE) từng trang một, không bao giờ sbrk(-...) để trả
    // lại trang trong lúc đang mở rộng — tránh việc bộ cấp phát trả lại
    // đúng những trang vừa giải phóng (như sbrk(-size) rồi sbrk(size) có
    // thể gây ra).
    char *base = sbrk(0);     // mốc đầu heap hiện tại, không cấp phát gì cả
    int scanned = 0;          // đã quét tới đâu tính từ base

    for (int page = 0; page < MAX_PAGES && total_found == 0; page++) {
      char *p = sbrk(PGSIZE);
      if (p == (char*)-1) {
        fprintf(2, "sbrk failed\n");
        exit(1);
      }
      int new_size = scanned + PGSIZE;

      // Chỉ quét phần mới cộng thêm msg_len-1 byte chồng lấn phía trước,
      // để không bỏ sót trường hợp tiền tố nằm vắt qua ranh giới trang cũ/mới.
      int from = scanned - (msg_len - 1);
      if (from < 0) from = 0;

      total_found += scan_range(base, from, new_size, msg, msg_len);
      scanned = new_size;
    }

    // Kết thúc một "đợt" MAX_PAGES trang: trả lại toàn bộ heap đã mượn
    // trong đợt này, để đợt sau sbrk(0) nhận một base mới và có cơ hội
    // được cấp các trang vật lý khác (không đảm bảo, nhưng tăng cơ hội
    // qua nhiều đợt thử độc lập thay vì lặp free/realloc cùng kích thước
    // ngay lập tức).
    if (total_found == 0) {
      sbrk(-(scanned));
    }
  }

  if (total_found == 0) {
    fprintf(2, "khong tim thay ung vien hop le sau %d lan thu\n", MAX_TRIES);
    exit(1);
  }

  exit(0);
}