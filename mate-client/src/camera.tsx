import { createRef, useEffect } from "react";
import { ORIGIN } from "./main";

export default function CameraComponent() {
	const imageRef = createRef<HTMLImageElement>();
	const parentRef = createRef<HTMLDivElement>();

	useEffect(() => {
		if (imageRef.current === undefined) return;

		const socket = new WebSocket("ws://" + ORIGIN + "/camera");
		socket.onmessage = (event) => {
			const reader = new FileReader();
			reader.onload = () => (imageRef.current!.src = reader.result as string);
			reader.readAsDataURL(event.data);
		};
		return () => socket.close();
	}, [imageRef]);

	return (
		<div ref={parentRef} className="w-full h-full">
			<img className="w-full h-full object-contain" ref={imageRef} />
		</div>
	);
}
