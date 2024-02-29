import React from "react";
import ReactDOM from "react-dom/client";

import { Mosaic, MosaicWindow } from "react-mosaic-component";

import "react-mosaic-component/react-mosaic-component.css";
import "./index.css";
import CameraComponent from "./camera";
import ThrusterControlComponent from "./thruster";
import BNODataComponent from "./bno";

export const COMPONENTS = {
	"Camera Stream": <CameraComponent />,
	"Thruster Control": <ThrusterControlComponent />,
	"BNO Data": <BNODataComponent />,
};

export const ORIGIN =
	import.meta.env.MODE == "development"
		? "192.168.86.248:3000"
		: window.location.host;

ReactDOM.createRoot(document.getElementById("root")!).render(
	<React.StrictMode>
		<Mosaic<keyof typeof COMPONENTS>
			className="w-screen h-screen"
			renderTile={(id, path) => (
				<MosaicWindow<keyof typeof COMPONENTS>
					toolbarControls={[]}
					title={id}
					path={path}
				>
					{COMPONENTS[id]}
				</MosaicWindow>
			)}
			initialValue={{
				direction: "row",
				first: {
					direction: "column",
					first: "Thruster Control",
					second: "BNO Data",
					splitPercentage: 40,
				},
				second: "Camera Stream",
				splitPercentage: 30,
			}}
		/>
	</React.StrictMode>,
);
