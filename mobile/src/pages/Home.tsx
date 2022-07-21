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
  IonToggle,
} from "@ionic/react";
import ExploreContainer from "../components/ExploreContainer";
import "./Home.css";
import {
  BleClient,
  BleDevice,
  numbersToDataView,
  numberToUUID,
  RequestBleDeviceOptions,
} from "@capacitor-community/bluetooth-le";
import { useEffect, useState } from "react";

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
    await BleClient.writeWithoutResponse(
      props.device.deviceId,
      SERVICES.TRIGGER,
      CHARACTERISTICS.TRIGGER_LOAD,
      numbersToDataView([checked ? 1 : 0])
    );
  };

  useEffect(() => {
    (async () => {
      await BleClient.connect(props.device.deviceId);

      await BleClient.startNotifications(
        props.device.deviceId,
        SERVICES.BATTERY,
        CHARACTERISTICS.BATTERY_LEVEL,
        (value) => {
          setBatteryLevel(value.getUint16(0));
        }
      );

      await BleClient.startNotifications(
        props.device.deviceId,
        SERVICES.PV,
        CHARACTERISTICS.PV_VOLTAGE,
        (value) => {
          setPVVoltage(value.getFloat32(0));
        }
      );

      await BleClient.startNotifications(
        props.device.deviceId,
        SERVICES.PV,
        CHARACTERISTICS.PV_CURRENT,
        (value) => {
          setPVCurrent(value.getFloat32(0));
        }
      );

      await BleClient.startNotifications(
        props.device.deviceId,
        SERVICES.PV,
        CHARACTERISTICS.PV_POWER,
        (value) => {
          setPVPower(value.getUint16(0));
        }
      );

      await BleClient.startNotifications(
        props.device.deviceId,
        SERVICES.TRIGGER,
        CHARACTERISTICS.TRIGGER_LOAD,
        (value) => {
          setLoadActive(value.getUint8(0) !== 0 ? true : false);
        }
      );
    })();
  }, [props.device.deviceId]);

  return (
    <IonContent>
      <IonList>
        <IonItem>
          <IonLabel>{props.device.name}</IonLabel>
        </IonItem>
        <IonItem>
          <IonLabel>PV Voltage</IonLabel>
          <IonLabel>{pvVoltage}</IonLabel>
        </IonItem>
        <IonItem>
          <IonLabel>PV Current</IonLabel>
          <IonLabel>{pvCurrent}</IonLabel>
        </IonItem>
        <IonItem>
          <IonLabel>PV Power</IonLabel>
          <IonLabel>{pvPower}</IonLabel>
        </IonItem>
        <IonItem>
          <IonLabel>Battery Level</IonLabel>
          <IonLabel>{batteryLevel}</IonLabel>
        </IonItem>
      </IonList>

      <IonItem>
        <IonLabel>Load</IonLabel>
        <IonToggle
          checked={loadActive}
          onIonChange={(e) => onTriggerLoad(e.detail.checked)}
        ></IonToggle>
      </IonItem>
    </IonContent>
  );
};

const Home: React.FC = () => {
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
      setError(error as string);
    }
  };

  return (
    <IonPage>
      <IonHeader>
        <IonToolbar>
          <IonTitle>Blank</IonTitle>
        </IonToolbar>
      </IonHeader>
      <IonContent fullscreen>
        <IonHeader collapse="condense">
          <IonToolbar>
            <IonTitle size="large">Blank</IonTitle>
          </IonToolbar>
        </IonHeader>
        <ExploreContainer />

        <p>{error}</p>
        {device ? (
          <SolarBox device={device}></SolarBox>
        ) : (
          <IonButton onClick={() => onDiscover()}>Discover</IonButton>
        )}
      </IonContent>
    </IonPage>
  );
};

export default Home;
