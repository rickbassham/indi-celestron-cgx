# APT repo for indi-celestron-cgx

## Celestron CGX USB driver

### All

```bash
wget -qO - https://rickbassham.github.io/indi-celestron-cgx/public.key | sudo apt-key add -
```

### Ubuntu Focal (amd64/arm64)

```bash
echo "deb https://rickbassham.github.io/indi-celestron-cgx/repos/apt/focal focal main" | \
    sudo tee /etc/apt/sources.list.d/indi-celestron-cgx.list
```

### Ubuntu Groovy (amd64/arm64)

```bash
echo "deb https://rickbassham.github.io/indi-celestron-cgx/repos/apt/groovy groovy main" | \
    sudo tee /etc/apt/sources.list.d/indi-celestron-cgx.list
```

### Raspbian Buster

```bash
echo "deb https://rickbassham.github.io/indi-celestron-cgx/repos/apt/raspbian buster main" | \
    sudo tee /etc/apt/sources.list.d/indi-celestron-cgx.list
```

### All

```bash
sudo apt update && sudo apt install indi-celestron-cgx
```
