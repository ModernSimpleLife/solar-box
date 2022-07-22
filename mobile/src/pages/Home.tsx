import {
  IonButton,
  IonContent,
  IonHeader,
  IonItem,
  IonList,
  IonPage,
  IonTitle,
  IonToolbar,
  IonLabel,
  IonIcon,
  IonText,
} from "@ionic/react";
import { power } from "ionicons/icons";
import "./Home.css";
import {
  BleClient,
  BleDevice,
  numbersToDataView,
  numberToUUID,
  RequestBleDeviceOptions,
} from "@capacitor-community/bluetooth-le";
import { useCallback, useEffect, useState } from "react";

const SERVICES = {
  BATTERY: numberToUUID(0x180f), // battery service
  PV: "b871a2ee-1651-47ac-a22c-e340d834c1ef", // pv service
  TRIGGER: "385cc70e-8a8c-4827-abc0-d01385aa0574", // trigger service
};

const CHARACTERISTICS = {
  BATTERY_LEVEL: numberToUUID(0x2a19),
  PV_VOLTAGE: "46e98325-92b7-4e5f-84c9-8edcbd9338db",
  PV_CURRENT: "91b3d4db-550b-464f-8127-16eeb209dd1d",
  PV_POWER: "2c85bbb9-0e1a-4bbb-8315-f7cc29831515",
  TRIGGER_LOAD: "287651ed-3fda-42f4-92c6-7aaca7da634c",
};

interface SolarBoxProps {
  device: BleDevice;
}

const SolarBox: React.FC<SolarBoxProps> = (props) => {
  const [loadActive, setLoadActive] = useState(false);
  const [batteryLevel, setBatteryLevel] = useState(0);
  const [pvVoltage, setPVVoltage] = useState(0);
  const [pvCurrent, setPVCurrent] = useState(0);
  const [pvPower, setPVPower] = useState(0);

  const onTriggerLoad = async (checked: boolean) => {
    console.log(`Trigger ${checked}`);
    await BleClient.writeWithoutResponse(
      props.device.deviceId,
      SERVICES.TRIGGER,
      CHARACTERISTICS.TRIGGER_LOAD,
      numbersToDataView([checked ? 1 : 0])
    );
  };

  const readData = useCallback(
    () =>
      Promise.all([
        BleClient.read(
          props.device.deviceId,
          SERVICES.BATTERY,
          CHARACTERISTICS.BATTERY_LEVEL
        ),
        BleClient.read(
          props.device.deviceId,
          SERVICES.PV,
          CHARACTERISTICS.PV_VOLTAGE
        ),
        BleClient.read(
          props.device.deviceId,
          SERVICES.PV,
          CHARACTERISTICS.PV_CURRENT
        ),
        BleClient.read(
          props.device.deviceId,
          SERVICES.PV,
          CHARACTERISTICS.PV_POWER
        ),
        BleClient.read(
          props.device.deviceId,
          SERVICES.TRIGGER,
          CHARACTERISTICS.TRIGGER_LOAD
        ),
      ]),
    [props.device.deviceId]
  );

  useEffect(() => {
    (async () => {
      await BleClient.connect(props.device.deviceId);

      const interval = setInterval(async () => {
        const data = await readData();
        setBatteryLevel(data[0].getUint16(0, true));
        setPVVoltage(data[1].getFloat32(0, true));
        setPVCurrent(data[2].getFloat32(0, true));
        setPVPower(data[3].getUint16(0, true));
        const loadActive = data[4].getUint8(0) !== 0 ? true : false;
        console.log("Confirm load active:", loadActive);
        setLoadActive(loadActive);
      }, 500);

      return () => {
        clearInterval(interval);
      };
    })();
  }, [props.device.deviceId, readData]);

  return (
    <IonPage>
      <IonHeader>
        <IonToolbar>
          <IonTitle>
            {props.device.name || ""} ({props.device.deviceId})
          </IonTitle>
        </IonToolbar>
      </IonHeader>
      <IonContent fullscreen>
        <IonHeader collapse="condense">
          <IonToolbar>
            <IonTitle size="large">
              {props.device.name || ""} ({props.device.deviceId})
            </IonTitle>
          </IonToolbar>
        </IonHeader>

        <IonContent>
          <IonList>
            <IonItem>
              <IonLabel>PV Voltage</IonLabel>
              <IonLabel>{pvVoltage.toFixed(2)} V</IonLabel>
            </IonItem>
            <IonItem>
              <IonLabel>PV Current</IonLabel>
              <IonLabel>{pvCurrent.toFixed(2)} A</IonLabel>
            </IonItem>
            <IonItem>
              <IonLabel>PV Power</IonLabel>
              <IonLabel>{pvPower} Watts</IonLabel>
            </IonItem>
            <IonItem>
              <IonLabel>Battery Level</IonLabel>
              <IonLabel>{batteryLevel}%</IonLabel>
            </IonItem>
          </IonList>

          <IonItem>
            <IonLabel>Load</IonLabel>
            <IonButton
              size="large"
              color={loadActive ? "danger" : "primary"}
              onClick={() => onTriggerLoad(!loadActive)}
            >
              <IonIcon icon={power}></IonIcon>
            </IonButton>
          </IonItem>
        </IonContent>
      </IonContent>
    </IonPage>
  );
};

const Home: React.FC = () => {
  const [bleAvailable, setBleAvailable] = useState(false);
  const [device, setDevice] = useState<BleDevice | null>(null);
  const [error, setError] = useState("");

  const onDiscover = async () => {
    try {
      const opts: RequestBleDeviceOptions = {
        namePrefix: "Solar Box",
        services: Object.values(SERVICES),
      };
      const device = await BleClient.requestDevice(opts);

      setDevice(device);
    } catch (error) {
      console.error(error);
      setError((error as Error).toString());
    }
  };

  useEffect(() => {
    (async () => {
      try {
        await BleClient.initialize({ androidNeverForLocation: true });
        setBleAvailable(true);
      } catch (error) {
        setBleAvailable(false);
        setError((error as Error).toString());
      }
    })();
  }, []);

  const Discovery = () => (
    <IonPage>
      <IonHeader>
        <IonToolbar>
          <IonTitle>Device Discovery</IonTitle>
        </IonToolbar>
      </IonHeader>
      <IonContent fullscreen>
        <IonHeader collapse="condense">
          <IonToolbar>
            <IonTitle size="large">Device Discovery</IonTitle>
          </IonToolbar>
        </IonHeader>
        <IonItem>
          <IonButton
            size="default"
            onClick={() => onDiscover()}
            disabled={!bleAvailable}
          >
            Discover
          </IonButton>
          <IonLabel slot="error">
            <IonText>{error}</IonText>
          </IonLabel>
        </IonItem>
      </IonContent>
    </IonPage>
  );

  return device ? <SolarBox device={device}></SolarBox> : <Discovery />;
};

export default Home;
