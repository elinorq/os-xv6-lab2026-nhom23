# Đồ án 1 – Xv6 Utilities & System calls

Nhóm: <số nhóm> · Deadline: Thứ 3, 20/10/2026 (nội bộ: 20:00 Thứ 2, 19/10)

## Thành viên
| Tên | MSSV | Vai trò |
|---|---|---|
| A | | memdump, find+exec, ráp report, đóng gói |
| B | | shell (3a, 3b, 3c) |
| C | | sandbox, attack |

## Cấu trúc repo
- `origin` = git://g.csail.mit.edu/xv6-labs-2026 (chỉ đọc, KHÔNG push)
- `team`   = repo này
- Branch tích hợp: `util` (lab 1), `syscall` (lab 2). Chỉ merge qua Pull Request.
- Branch tính năng: `feat-util-*`, `feat-syscall-*`, `bonus-bug`
- Branch `main`: chỉ chứa tài liệu, KHÔNG đưa vào patch.

## Quy trình ngắn
1. `git checkout util && git pull team util`
2. `git checkout -b feat-util-<tên>`; code, test, commit `[bài] mô tả`
3. `git push team feat-util-<tên>`, mở PR vào `util`
4. Người khác chạy `make clean && make grade`, approve, rồi merge

## Tiến độ
| Bài | Người | Trạng thái | Test |
|---|---|---|---|
| 1 memdump | A | ☐ | |
| 2 find + exec | A | ☐ | |
| 3a/3b/3c shell | B | ☐ | demo tay |
| 4 sandbox | C | ☐ | |
| 5 attack | C | ☐ | |
| 6 bonus | cả nhóm | ☐ | |

## Lệnh hữu ích
`make qemu` · `make clean` · `make grade` · `./grade-lab-util <test>` · `./grade-lab-syscall <test>`

## Tài liệu
Hướng dẫn chi tiết: <dán link doc nhóm>
