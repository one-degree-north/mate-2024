import { useEffect, useState } from "react";
import { ORIGIN } from "./main";

type BNOData = {
	euler: number[];
	quaternion: number[];
	linear_acceleration: number[];
	magnetometer: number[];
	gyroscope: number[];
	acceleration: number[];
	gravity: number[];
};

export default function BNODataComponent() {
	const [data, setData] = useState<BNOData>();

	useEffect(() => {
		const events = new EventSource("http://" + ORIGIN + "/data");
		events.onmessage = (event) => setData(JSON.parse(event.data));
		events.onopen = () => console.log("opened data stream");
		events.onerror = (err) => console.log(err);

		return () => events.close();
	}, []);

	return (
		<div className="w-full h-full p-4">
			<div>Euler Angles: {data?.euler.join(" ")}</div>
			<div>Quaternion: {data?.quaternion.join(" ")}</div>
			<div>Linear Acceleration: {data?.acceleration.join(" ")}</div>
			<div>Magnetometer: {data?.magnetometer.join(" ")}</div>
			<div>Gyroscope: {data?.gyroscope.join(" ")}</div>
			<div>Acceleration: {data?.acceleration.join(" ")}</div>
			<div>Gravity: {data?.acceleration.join(" ")}</div>
		</div>
	);
}
