FROM ubuntu:jammy

RUN apt-get update && \
    apt-get install -y tmux zsh curl git make gcc && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" --unattended

RUN sed -i 's/ZSH_THEME=".*"/ZSH_THEME="dieter"/' ~/.zshrc

COPY watcher-flush_reload/ /root/watcher-flush_reload/

WORKDIR /root/watcher-flush_reload

RUN make

CMD ["zsh"]
