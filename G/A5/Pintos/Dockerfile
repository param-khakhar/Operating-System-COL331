FROM ubuntu:18.04

RUN useradd -m pintos && echo pintos:pintos | chpasswd && \
	usermod -aG sudo pintos

# General-purpose utilities
RUN apt-get update && apt-get -y install \
	bash \
	git \
	build-essential \
	gdb \
	gcc \
	emacs \
	vim \
	nano \
	sudo \
        qemu

# Switch user
USER pintos
WORKDIR /home/pintos

# Clone pintos - needs adjustment to the right repo
RUN git clone https://git.cs.lth.se/al7330du/pintos-public pintos

RUN /bin/bash -c 'echo "pushd /home/pintos/pintos > /dev/null ; source setup.sh ; popd > /dev/null" >> /home/pintos/.bashrc'

CMD /bin/bash
