# Dự án EduDoom-Ass — Phiên bản mkdocs

> **Cập nhật lần cuối:** 2026-11-14
> 
> 🚀 **Trạng thái:** Nâng cấp thành công lên Nấc **Level 1A** (Kiến trúc 2 Cụm - 5 Nấc).
> File này được tự động tạo bởi `setup_env.bat` từ template `kit\templates\README.root.md`. Nếu bạn có ghi chú cá nhân, hãy sao lưu trước khi chạy lại script.

## 🌟 Tính năng của Nấc 1A (v2.1.0)
- **Âm thanh & Nhạc nền:** Kích hoạt thành công hệ thống âm thanh Stereo và nhạc nền qua `SDL2_mixer`.
- **Đồ họa độ phân giải cao:** Nâng độ phân giải nội tại, tích hợp bộ lọc hậu kỳ OpenGL (Nearest/Linear/CRT).
- **Tối ưu hóa:** Biên dịch với cờ tối ưu vi kiến trúc Zen 3 (`-march=znver3`), khai thác trọn vẹn 96MB L3 Cache của Ryzen 5 5500X3D.
- **Tương thích hoàn hảo:** Tệp lưu màn chơi `.dsg` từ v2.0.3 vẫn sử dụng được 100%.

## 🗺️ Điều hướng Tài liệu
Dưới đây là danh sách các tài liệu trong kho `assets\document\`. Bạn có thể tự do thêm file mới, hệ thống sẽ tự động cập nhật lại danh sách này vào lần chạy tiếp theo.

*Chưa có tài liệu nào.*

---
## 🛠️ Hướng dẫn Build & Phát triển
1. Mở dự án bằng **Geany IDE**: Nhấp đúp vào file `EduDoom-Ass.geany`.
2. Sử dụng 4 nút lệnh trên thanh công cụ: **Build**, **Clean**, **Run**, và **Generate Docs**.
3. Mọi cấu hình biên dịch (Makefile) nằm trong thư mục `Custom build Makefile\`.
4. Mọi thư viện bên thứ ba (SDL2, SDL2_mixer) nằm gọn trong `third_party\`. Không nhét lẫn vào `mingw64\`.

Chúc bạn có những trải nghiệm tuyệt vời cùng EduDoom-Ass!