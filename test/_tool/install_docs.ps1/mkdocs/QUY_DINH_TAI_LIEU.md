# QUY ĐỊNH VIẾT TÀI LIỆU (DÀNH CHO LLM VÀ NGƯỜI DÙNG)

> **CẢNH BÁO:** File này là "luật". Mọi file `NN_*.md` (từ `01_` đến `30_`) trong thư mục `assets\document\` đều đã bị ĐÓNG BĂNG. Script cài đặt (`install_docs.ps1`) TUYỆT ĐỐI KHÔNG ghi đè, không sửa, không xóa các file này.

## 1. Nguyên tắc cốt lõi
1. **Bạn tự do:** Bạn có thể tự do thêm mới, sửa đổi, hoặc xóa các file `NN_*.md` trong `assets\document\`.
2. **Tự cập nhật bản đồ:** `assets\document\README.md` (bản đồ con) và `README.md` ở gốc dự án sẽ tự động được script quét và dựng lại mỗi khi bạn chạy `setup_env.bat`.
3. **Không cần đánh số lại:** Nếu bạn thêm file mới, cứ đặt tên theo định dạng `XX_Ten_File.md` (ví dụ `31_Huong_Dan_Moi.md`). Script sẽ tự nhận diện.
4. **Không có cơ chế state-hash:** Không có file `.docs_state.json`, không sinh ra file `.new.md`. Script không quan tâm bạn đã sửa gì trong `NN_*.md`, nó chỉ quét tên file để dựng mục lục.

## 2. Cấu trúc hiển thị của MkDocs
Mọi file `.md` khi đưa lên MkDocs sẽ được hiển thị dựa trên cấu trúc tiêu đề (Heading):
- Dùng `#` cho Tiêu đề cấp 1 (H1) - Tên file.
- Dùng `##` cho Tiêu đề cấp 2 (H2) - Các mục lớn.
- Dùng `###` cho Tiêu đề cấp 3 (H3) - Các mục nhỏ.

## 3. Biểu mẫu tối thiểu
*Khi tạo một file `NN_*.md` mới, hãy copy cấu trúc mẫu này:*

```markdown
# [Số hiệu] - [Tên Tài Liệu]

## 1. Mục tiêu
(Mô tả ngắn gọn tài liệu này dùng để làm gì)

## 2. Nội dung chính
(Chi tiết kỹ thuật, hướng dẫn, hoặc ghi chú)

## 3. Tài liệu tham khảo
(Các file hoặc link liên quan)
```