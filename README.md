# Đồ án 1 – Xv6 Utilities & System calls

Tài liệu dành cho người nhận bàn giao và người chấm Đồ án 1 của Nhóm 23, môn CSC10007 – Hệ điều hành. README hướng dẫn lấy mã nguồn, chạy năm bài chính và kiểm tra bản vá Bonus `kill(1)`.

## Thành viên và phân công

| Họ và tên | MSSV | Phần phụ trách |
|---|---|---|
| Ngô Minh Đức | 24120007 | Trưởng nhóm; Bài 3: shell (3a, 3b, 3c); Bài 6: Bonus `kill(1)` |
| Phạm Đức Toàn | 24120467 | Bài 1: `memdump`; Bài 2: `find` và `-exec` |
| Đỗ Ngọc Tường Quyên | 24120423 | Bài 4: Sandbox; Bài 5: Attack |

## Nơi lưu bài làm

| Nhánh | Nội dung | Revision đối chiếu ngày 23/10/2026 |
|---|---|---|
| [`main`](https://github.com/elinorq/os-xv6-lab2026-nhom23/tree/main) | Tài liệu; không chứa mã nguồn để build xv6 | `d2202e8` trước cập nhật README này |
| [`util`](https://github.com/elinorq/os-xv6-lab2026-nhom23/tree/util) | Bài 1–3; `conf/lab.mk` đặt `LAB=util` | `b8e15b2` |
| [`syscall`](https://github.com/elinorq/os-xv6-lab2026-nhom23/tree/syscall) | Bài 4–5 và bản vá Bonus; `conf/lab.mk` đặt `LAB=syscall` | `7076b27` |

Các revision trên được đối chiếu từ Git local. Chúng xác định trạng thái mã nguồn được rà soát, không phải xác nhận các nhánh trên GitHub sẽ luôn giữ nguyên revision đó. Mỗi nhánh mã nguồn có grader riêng; không chạy cả hai grader trong cùng một checkout.

## Môi trường và cách lấy mã nguồn

Chạy các lệnh dưới đây trong terminal Linux, chẳng hạn Ubuntu trên WSL2. Cần Git, GNU Make, Python 3, `bc`, bộ công cụ biên dịch chéo RISC-V (GCC và Binutils) và `qemu-system-riscv64`. Makefile hỗ trợ các tiền tố `riscv64-unknown-elf-`, `riscv64-linux-gnu-` hoặc `riscv64-unknown-linux-gnu-` và yêu cầu QEMU từ 7.2.

Từ cùng một thư mục cha, tạo hai checkout riêng:

```sh
git clone --branch util \
  https://github.com/elinorq/os-xv6-lab2026-nhom23.git \
  xv6-util
git clone --branch syscall \
  https://github.com/elinorq/os-xv6-lab2026-nhom23.git \
  xv6-syscall
```

Trước khi chạy, dùng `git rev-parse --short HEAD` để ghi nhận revision thực tế và `cat conf/lab.mk` để kiểm tra lab đang chọn. Các bước tiếp theo đều chạy trong checkout tương ứng.

## Chạy và kiểm thử

### Lab Utilities – trong thư mục `xv6-util`

Kiểm tra từng phần bằng grader của nhánh `util`:

```sh
make clean
./grade-lab-util memdump
./grade-lab-util find
./grade-lab-util exec
```

Demo shell bằng `make qemu`. Trong shell xv6, kiểm tra prompt khi chạy `sh < findtest.sh`, Tab completion (ví dụ `ec<Tab>`, `cat REA<Tab>`) và history (`history`, `!!`, `!n`, phím lên/xuống). Đây là các kiểm tra thủ công, không thay thế kiểm thử hồi quy.

### Lab System calls – trong thư mục `xv6-syscall`

Kiểm tra Sandbox và Attack bằng grader của nhánh `syscall`:

```sh
make clean
./grade-lab-syscall sandbox
./grade-lab-syscall attack
```

Bộ lọc `sandbox` chạy cả năm test theo đề và ba test bổ sung có sẵn trong grader của nhánh. Không cộng chúng thành tám test bắt buộc của đề.

Để kiểm tra bản vá Bonus, chạy `make CPUS=1 qemu`, rồi gõ `killinit` trong shell xv6. Trên bản đã vá, PoC phải in `blocked`, sau đó `alive`. Nhánh `syscall` hiện chứa bản vá; đây là demo sau vá, không phải cách tái hiện panic trước vá.

### Demo và toàn bộ suite

`make qemu` mở xv6 để demo tương tác. Thoát QEMU bằng cách nhấn `Ctrl+A`, thả phím rồi nhấn `X` trước khi chạy lệnh tiếp theo ở terminal Linux.

Trong từng checkout, `make grade` chạy toàn bộ suite theo `conf/lab.mk` và tự gọi `make clean`. Suite còn có các mục ngoài năm bài chính hoặc kiểm tra tệp như `time.txt`, `answers-syscall.txt` tùy lab. Kết quả của một bộ lọc không đồng nghĩa với toàn bộ suite PASS.

## Trạng thái hiện thực và bằng chứng kiểm thử

Các chức năng trong bảng đã có mã nguồn trên nhánh tương ứng. Kết quả dưới đây được ghi nhận trong báo cáo và ảnh minh chứng; không phải một lượt chạy mới trên các revision đối chiếu ở trên.

| Phần | Nơi lưu hiện thực | Bằng chứng và giới hạn |
|---|---|---|
| Bài 1: `memdump` | `util` | Ảnh tổng kết ghi 3/3 test `OK`. |
| Bài 2: `find` và `-exec` | `util` | Ảnh tổng kết ghi 7/7 test `OK`, nhưng `exec via sh script` có `Timeout!`; chưa đạt tiêu chí không timeout. |
| Bài 3: shell | `util` | Báo cáo ghi nhận demo ba nhóm chức năng; không có suite riêng theo đề. Kiểm thử hồi quy script còn hiện tượng timeout. |
| Bài 4: Sandbox | `syscall` | Ảnh tổng kết ghi `OK` cho 5 test theo đề và 3 test bổ sung. |
| Bài 5: Attack | `syscall` | Ảnh tổng kết ghi `attack: OK`; báo cáo có hai demo thủ công. Kết quả không bảo đảm thành công với mọi trạng thái cấp phát bộ nhớ. |
| Bài 6: Bonus `kill(1)` | `syscall` | Ảnh trước/sau với `CPUS=1` ghi nhận panic trước vá và PoC bị chặn sau vá; chưa xác nhận toàn bộ suite sau tích hợp Bonus. |

Ảnh kiểm thử riêng của Bài 2 ghi `exec via sh script: OK` không kèm timeout, còn ảnh tổng kết có timeout. Các ảnh không ghi revision nên chưa thể coi chúng là kết quả của cùng một bản build. Báo cáo LaTeX bàn giao trình bày chi tiết các trường hợp và giới hạn này. Điểm tự chấm trong báo cáo là đánh giá của nhóm, không phải điểm chính thức.
