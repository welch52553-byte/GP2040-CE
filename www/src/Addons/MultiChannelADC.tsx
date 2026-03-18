import { useContext, useEffect, useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { Button, Col, FormCheck, Modal, ProgressBar, Row } from 'react-bootstrap';
import * as yup from 'yup';

import Section from '../Components/Section';
import FormSelect from '../Components/FormSelect';
import FormControl from '../Components/FormControl';
import { ANALOG_PINS } from '../Data/Buttons';
import AnalogPinOptions from '../Components/AnalogPinOptions';
import { AppContext } from '../Contexts/AppContext';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';
import WebApi from '../Services/WebApi';

const ADC_MAX = 4095;

const CHANNEL_NAMES = ['Steer Left', 'Steer Right', 'Throttle', 'Brake'];
const REST_KEYS = ['mcadcSteerLeftRest', 'mcadcSteerRightRest', 'mcadcThrottleRest', 'mcadcBrakeRest'];
const ACTIVE_KEYS = ['mcadcSteerLeftActive', 'mcadcSteerRightActive', 'mcadcThrottleActive', 'mcadcBrakeActive'];

export const multiChannelADCScheme = {
	MultiChannelADCEnabled: yup.number().required().label('Multi-Channel ADC Enabled'),
	mcadcSteerLeftPin: yup.number().label('Steer Left Pin').validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcSteerRightPin: yup.number().label('Steer Right Pin').validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcThrottlePin: yup.number().label('Throttle Pin').validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcBrakePin: yup.number().label('Brake Pin').validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcSteerLeftRest: yup.number().label('Steer Left Rest').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcSteerLeftActive: yup.number().label('Steer Left Active').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcSteerRightRest: yup.number().label('Steer Right Rest').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcSteerRightActive: yup.number().label('Steer Right Active').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcThrottleRest: yup.number().label('Throttle Rest').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcThrottleActive: yup.number().label('Throttle Active').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcBrakeRest: yup.number().label('Brake Rest').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcBrakeActive: yup.number().label('Brake Active').validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcDeadzone: yup.number().label('Deadzone (%)').validateRangeWhenValue('MultiChannelADCEnabled', 0, 50),
	mcadcSmoothingFactor: yup.number().label('Smoothing Factor').validateRangeWhenValue('MultiChannelADCEnabled', 1, 999),
	mcadcOversampling: yup.number().label('Oversampling').validateRangeWhenValue('MultiChannelADCEnabled', 1, 16),
	mcadcAutoCalibrate: yup.number().label('Auto Calibrate').validateRangeWhenValue('MultiChannelADCEnabled', 0, 1),
};

export const multiChannelADCState = {
	MultiChannelADCEnabled: 0,
	mcadcSteerLeftPin: -1,
	mcadcSteerRightPin: -1,
	mcadcThrottlePin: -1,
	mcadcBrakePin: -1,
	mcadcSteerLeftRest: 2414,
	mcadcSteerLeftActive: 4076,
	mcadcSteerRightRest: 2414,
	mcadcSteerRightActive: 4076,
	mcadcThrottleRest: 2414,
	mcadcThrottleActive: 4076,
	mcadcBrakeRest: 2414,
	mcadcBrakeActive: 4076,
	mcadcDeadzone: 3,
	mcadcSmoothingFactor: 100,
	mcadcOversampling: 4,
	mcadcAutoCalibrate: 1,
};

type CalibrationModalProps = {
	show: boolean;
	channelIndex: number;
	onClose: () => void;
	onSave: (rest: number, active: number) => void;
};

const CalibrationModal = ({ show, channelIndex, onClose, onSave }: CalibrationModalProps) => {
	const [step, setStep] = useState(0);
	const [voltage, setVoltage] = useState(0);
	const [restValue, setRestValue] = useState(0);
	const [activeValue, setActiveValue] = useState(ADC_MAX);
	const timerId = useRef<number>();

	const readADC = async () => {
		try {
			const result = await WebApi.getMcadcVoltage({ channel: channelIndex });
			if (result?.data?.voltage !== undefined) {
				setVoltage(result.data.voltage);
			}
		} catch (e) {
			console.error('ADC read failed:', e);
		}
	};

	useEffect(() => {
		if (show) {
			setStep(0);
			setVoltage(0);
			setRestValue(0);
			setActiveValue(ADC_MAX);
			const id = setInterval(readADC, 80);
			timerId.current = id;
		}
		return () => {
			if (timerId.current) clearInterval(timerId.current);
		};
	}, [show, channelIndex]);

	const pct = (voltage / ADC_MAX) * 100;

	const activationPct = restValue !== activeValue
		? Math.max(0, Math.min(100, ((voltage - restValue) / (activeValue - restValue)) * 100))
		: 0;

	return (
		<Modal centered show={show} onHide={onClose}>
			<Modal.Header closeButton>
				<Modal.Title>Calibrate: {CHANNEL_NAMES[channelIndex]}</Modal.Title>
			</Modal.Header>
			<Modal.Body>
				{step === 0 && (
					<>
						<p><strong>Step 1:</strong> Release the key completely, then click "Set Idle".</p>
						<p>Current ADC reading:</p>
						<ProgressBar>
							<ProgressBar variant="info" now={pct} />
						</ProgressBar>
						<h3 className="mt-2">{voltage}</h3>
					</>
				)}
				{step === 1 && (
					<>
						<p><strong>Step 2:</strong> Press the key fully down, then click "Set Pressed".</p>
						<p>Idle value: <strong>{restValue}</strong></p>
						<p>Current ADC reading:</p>
						<ProgressBar>
							<ProgressBar variant="warning" now={pct} />
						</ProgressBar>
						<h3 className="mt-2">{voltage}</h3>
					</>
				)}
				{step === 2 && (
					<>
						<p><strong>Step 3:</strong> Test - press and release the key to verify.</p>
						<p>Idle: <strong>{restValue}</strong> | Pressed: <strong>{activeValue}</strong> | Range: <strong>{Math.abs(activeValue - restValue)}</strong></p>
						<p>Live activation:</p>
						<ProgressBar>
							<ProgressBar variant={activationPct > 10 ? 'success' : 'secondary'} now={activationPct} />
						</ProgressBar>
						<h3 className="mt-2">{voltage} ({Math.round(activationPct)}%)</h3>
						<Row className="mt-3">
							<FormControl
								type="number"
								label="Rest (idle)"
								name="calRest"
								className="form-control-sm"
								groupClassName="col-sm-6"
								value={restValue}
								onChange={(e) => setRestValue(parseInt(e.target.value) || 0)}
								min={0}
								max={ADC_MAX}
							/>
							<FormControl
								type="number"
								label="Active (pressed)"
								name="calActive"
								className="form-control-sm"
								groupClassName="col-sm-6"
								value={activeValue}
								onChange={(e) => setActiveValue(parseInt(e.target.value) || 0)}
								min={0}
								max={ADC_MAX}
							/>
						</Row>
					</>
				)}
			</Modal.Body>
			<Modal.Footer>
				{step === 0 && (
					<Button onClick={() => { setRestValue(voltage); setStep(1); }}>
						Set Idle ({voltage})
					</Button>
				)}
				{step === 1 && (
					<>
						<Button variant="secondary" onClick={() => setStep(0)}>Back</Button>
						<Button onClick={() => { setActiveValue(voltage); setStep(2); }}>
							Set Pressed ({voltage})
						</Button>
					</>
				)}
				{step === 2 && (
					<>
						<Button variant="secondary" onClick={() => setStep(0)}>Restart</Button>
						<Button variant="success" onClick={() => {
							onSave(restValue, activeValue);
							onClose();
						}}>
							Save Calibration
						</Button>
					</>
				)}
			</Modal.Footer>
		</Modal>
	);
};

const MultiChannelADC = ({ values, errors, handleChange, handleCheckbox, setFieldValue }: AddonPropTypes) => {
	const { usedPins } = useContext(AppContext);
	const { t } = useTranslation('');
	const availableAnalogPins = ANALOG_PINS.filter((pin) => !usedPins?.includes(pin));

	const [calChannel, setCalChannel] = useState(-1);
	const [showCal, setShowCal] = useState(false);

	const openCalibration = (ch: number) => {
		setCalChannel(ch);
		setShowCal(true);
	};

	const saveCalibration = (rest: number, active: number) => {
		if (calChannel >= 0 && calChannel < 4) {
			setFieldValue(REST_KEYS[calChannel], rest);
			setFieldValue(ACTIVE_KEYS[calChannel], active);
		}
	};

	return (
		<Section title="Multi-Channel ADC (Sim Racing)">
			<div id="MultiChannelADCOptions" hidden={!values.MultiChannelADCEnabled}>
				<div className="alert alert-info" role="alert">
					Sim Racing Hall-Effect Controller: 4 linear hall keys for steering (left/right), throttle, and brake.
					Steering maps to Left Stick X. Throttle → Right Trigger. Brake → Left Trigger.
				</div>

				<h6 className="mt-3 mb-2">Pin Assignments</h6>
				<Row className="mb-3">
					{['mcadcSteerLeftPin', 'mcadcSteerRightPin', 'mcadcThrottlePin', 'mcadcBrakePin'].map((name, i) => (
						<FormSelect
							key={name}
							label={CHANNEL_NAMES[i] + ' Pin'}
							name={name}
							className="form-select-sm"
							groupClassName="col-sm-3 mb-3"
							value={values[name]}
							error={errors[name]}
							isInvalid={Boolean(errors[name])}
							onChange={handleChange}
						>
							<AnalogPinOptions />
						</FormSelect>
					))}
				</Row>

				<h6 className="mt-3 mb-2">Hall Sensor Calibration</h6>
				<div className="alert alert-warning" role="alert">
					<small>
						Click <strong>"Calibrate"</strong> for each channel to start live calibration.
						The wizard reads the ADC in real-time and guides you through idle → pressed → verify.
					</small>
				</div>
				{CHANNEL_NAMES.map((name, i) => (
					<Row key={name} className="mb-2 align-items-end">
						<div className="col-sm-2"><strong>{name}</strong></div>
						<FormControl
							type="number"
							label="Rest"
							name={REST_KEYS[i]}
							className="form-control-sm"
							groupClassName="col-sm-2"
							value={values[REST_KEYS[i]]}
							error={errors[REST_KEYS[i]]}
							isInvalid={Boolean(errors[REST_KEYS[i]])}
							onChange={handleChange}
							min={0}
							max={4095}
						/>
						<FormControl
							type="number"
							label="Active"
							name={ACTIVE_KEYS[i]}
							className="form-control-sm"
							groupClassName="col-sm-2"
							value={values[ACTIVE_KEYS[i]]}
							error={errors[ACTIVE_KEYS[i]]}
							isInvalid={Boolean(errors[ACTIVE_KEYS[i]])}
							onChange={handleChange}
							min={0}
							max={4095}
						/>
						<div className="col-sm-2">
							<Button
								size="sm"
								variant="outline-primary"
								onClick={() => openCalibration(i)}
							>
								Calibrate
							</Button>
						</div>
						<div className="col-sm-3 small text-muted">
							Range: {Math.abs(values[ACTIVE_KEYS[i]] - values[REST_KEYS[i]])}
						</div>
					</Row>
				))}

				<h6 className="mt-4 mb-2">Signal Processing</h6>
				<Row className="mb-3">
					<FormControl
						type="number"
						label="Deadzone (%)"
						name="mcadcDeadzone"
						className="form-control-sm"
						groupClassName="col-sm-2 mb-3"
						value={values.mcadcDeadzone}
						error={errors.mcadcDeadzone}
						isInvalid={Boolean(errors.mcadcDeadzone)}
						onChange={handleChange}
						min={0}
						max={50}
					/>
					<FormControl
						type="number"
						label="Smoothing (1-999)"
						name="mcadcSmoothingFactor"
						className="form-control-sm"
						groupClassName="col-sm-2 mb-3"
						value={values.mcadcSmoothingFactor}
						error={errors.mcadcSmoothingFactor}
						isInvalid={Boolean(errors.mcadcSmoothingFactor)}
						onChange={handleChange}
						min={1}
						max={999}
					/>
					<FormControl
						type="number"
						label="Oversampling (1-16)"
						name="mcadcOversampling"
						className="form-control-sm"
						groupClassName="col-sm-2 mb-3"
						value={values.mcadcOversampling}
						error={errors.mcadcOversampling}
						isInvalid={Boolean(errors.mcadcOversampling)}
						onChange={handleChange}
						min={1}
						max={16}
					/>
				</Row>
				<FormCheck
					label="Auto-Calibrate Rest on Startup"
					type="switch"
					id="mcadcAutoCalibrate"
					className="col-sm-4 ms-3 mb-3"
					isInvalid={false}
					checked={Boolean(values.mcadcAutoCalibrate)}
					onChange={(e) => {
						handleCheckbox('mcadcAutoCalibrate');
						handleChange(e);
					}}
				/>

				<CalibrationModal
					show={showCal}
					channelIndex={calChannel}
					onClose={() => setShowCal(false)}
					onSave={saveCalibration}
				/>
			</div>
			<FormCheck
				label={t('Common:switch-enabled')}
				type="switch"
				id="MultiChannelADCButton"
				reverse
				isInvalid={false}
				checked={Boolean(values.MultiChannelADCEnabled)}
				onChange={(e) => {
					handleCheckbox('MultiChannelADCEnabled');
					handleChange(e);
				}}
			/>
		</Section>
	);
};

export default MultiChannelADC;
