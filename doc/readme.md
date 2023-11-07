
### 工具安装

---

```bash
sudo apt-get install valgrind:i386
```

```bash
valgrind --leak-check=full ./your_application
valgrind --leak-check=full --show-leak-kinds=all ./your_32bit_application
```