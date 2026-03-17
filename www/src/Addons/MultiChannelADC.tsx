import { useContext } from 'react';
import { useTranslation } from 'react-i18next';
import { FormCheck, Row } from 'react-bootstrap';
import * as yup from 'yup';

import Section from '../Components/Section';
import FormSelect from '../Components/FormSelect';
import FormControl from '../Components/FormControl';
import { ANALOG_PINS } from '../Data/Buttons';
import AnalogPinOptions from '../Components/AnalogPinOptions';
import { AppContext } from '../Contexts/AppContext';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';

export const multiChannelADCScheme = {
	MultiChannelADCEnabled: yup.number().required().label('Multi-Channel ADC Enabled'),
	mcadcSteerLeftPin: yup
		.number()
		.label('Steer Left Pin')
		.validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcSteerRightPin: yup
		.number()
		.label('Steer Right Pin')
		.validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcThrottlePin: yup
		.number()
		.label('Throttle Pin')
		.validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcBrakePin: yup
		.number()
		.label('Brake Pin')
		.validatePinWhenValue('MultiChannelADCEnabled'),
	mcadcSteerLeftRest: yup
		.number()
		.label('Steer Left Rest')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcSteerLeftActive: yup
		.number()
		.label('Steer Left Active')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcSteerRightRest: yup
		.number()
		.label('Steer Right Rest')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcSteerRightActive: yup
		.number()
		.label('Steer Right Active')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcThrottleRest: yup
		.number()
		.label('Throttle Rest')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcThrottleActive: yup
		.number()
		.label('Throttle Active')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcBrakeRest: yup
		.number()
		.label('Brake Rest')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcBrakeActive: yup
		.number()
		.label('Brake Active')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 4095),
	mcadcDeadzone: yup
		.number()
		.label('Deadzone (%)')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 50),
	mcadcSmoothingFactor: yup
		.number()
		.label('Smoothing Factor')
		.validateRangeWhenValue('MultiChannelADCEnabled', 1, 999),
	mcadcOversampling: yup
		.number()
		.label('Oversampling')
		.validateRangeWhenValue('MultiChannelADCEnabled', 1, 16),
	mcadcAutoCalibrate: yup
		.number()
		.label('Auto Calibrate')
		.validateRangeWhenValue('MultiChannelADCEnabled', 0, 1),
};

export const multiChannelADCState = {
	MultiChannelADCEnabled: 0,
	mcadcSteerLeftPin: -1,
	mcadcSteerRightPin: -1,
	mcadcThrottlePin: -1,
	mcadcBrakePin: -1,
	mcadcSteerLeftRest: 512,
	mcadcSteerLeftActive: 3584,
	mcadcSteerRightRest: 512,
	mcadcSteerRightActive: 3584,
	mcadcThrottleRest: 512,
	mcadcThrottleActive: 3584,
	mcadcBrakeRest: 512,
	mcadcBrakeActive: 3584,
	mcadcDeadzone: 3,
	mcadcSmoothingFactor: 100,
	mcadcOversampling: 4,
	mcadcAutoCalibrate: 1,
};

const CalibrationRow = ({ label, restName, activeName, values, errors, handleChange }) => (
	<Row className="mb-2 align-items-end">
		<div className="col-sm-2">
			<strong>{label}</strong>
		</div>
		<FormControl
			type="number"
			label="Rest (idle)"
			name={restName}
			className="form-control-sm"
			groupClassName="col-sm-2"
			value={values[restName]}
			error={errors[restName]}
			isInvalid={Boolean(errors[restName])}
			onChange={handleChange}
			min={0}
			max={4095}
		/>
		<FormControl
			type="number"
			label="Active (pressed)"
			name={activeName}
			className="form-control-sm"
			groupClassName="col-sm-2"
			value={values[activeName]}
			error={errors[activeName]}
			isInvalid={Boolean(errors[activeName])}
			onChange={handleChange}
			min={0}
			max={4095}
		/>
	</Row>
);

const MultiChannelADC = ({ values, errors, handleChange, handleCheckbox }: AddonPropTypes) => {
	const { usedPins } = useContext(AppContext);
	const { t } = useTranslation('');
	const availableAnalogPins = ANALOG_PINS.filter(
		(pin) => !usedPins?.includes(pin),
	);

	return (
		<Section title="Multi-Channel ADC (Sim Racing)">
			<div id="MultiChannelADCOptions" hidden={!values.MultiChannelADCEnabled}>
				<div className="alert alert-info" role="alert">
					Sim Racing Hall-Effect Controller: 4 linear hall keys for steering (left/right), throttle, and brake.
					Maps to XInput Left Stick X + Left/Right Triggers.
				</div>
				<div className="alert alert-success" role="alert">
					Available ADC pins: {availableAnalogPins.join(', ')}
				</div>

				<h6 className="mt-3 mb-2">Pin Assignments</h6>
				<Row className="mb-3">
					<FormSelect
						label="Steer Left Pin"
						name="mcadcSteerLeftPin"
						className="form-select-sm"
						groupClassName="col-sm-3 mb-3"
						value={values.mcadcSteerLeftPin}
						error={errors.mcadcSteerLeftPin}
						isInvalid={Boolean(errors.mcadcSteerLeftPin)}
						onChange={handleChange}
					>
						<AnalogPinOptions />
					</FormSelect>
					<FormSelect
						label="Steer Right Pin"
						name="mcadcSteerRightPin"
						className="form-select-sm"
						groupClassName="col-sm-3 mb-3"
						value={values.mcadcSteerRightPin}
						error={errors.mcadcSteerRightPin}
						isInvalid={Boolean(errors.mcadcSteerRightPin)}
						onChange={handleChange}
					>
						<AnalogPinOptions />
					</FormSelect>
					<FormSelect
						label="Throttle Pin"
						name="mcadcThrottlePin"
						className="form-select-sm"
						groupClassName="col-sm-3 mb-3"
						value={values.mcadcThrottlePin}
						error={errors.mcadcThrottlePin}
						isInvalid={Boolean(errors.mcadcThrottlePin)}
						onChange={handleChange}
					>
						<AnalogPinOptions />
					</FormSelect>
					<FormSelect
						label="Brake Pin"
						name="mcadcBrakePin"
						className="form-select-sm"
						groupClassName="col-sm-3 mb-3"
						value={values.mcadcBrakePin}
						error={errors.mcadcBrakePin}
						isInvalid={Boolean(errors.mcadcBrakePin)}
						onChange={handleChange}
					>
						<AnalogPinOptions />
					</FormSelect>
				</Row>

				<h6 className="mt-3 mb-2">Hall Sensor Calibration (ADC 0-4095)</h6>
				<div className="alert alert-warning" role="alert">
					<small>
						<strong>Rest</strong> = ADC value when key is NOT pressed.
						<strong> Active</strong> = ADC value when key is FULLY pressed.
						{Boolean(values.mcadcAutoCalibrate) &&
							' Auto-calibrate is ON: rest values are measured on startup.'}
					</small>
				</div>
				<CalibrationRow
					label="Steer Left"
					restName="mcadcSteerLeftRest"
					activeName="mcadcSteerLeftActive"
					values={values}
					errors={errors}
					handleChange={handleChange}
				/>
				<CalibrationRow
					label="Steer Right"
					restName="mcadcSteerRightRest"
					activeName="mcadcSteerRightActive"
					values={values}
					errors={errors}
					handleChange={handleChange}
				/>
				<CalibrationRow
					label="Throttle"
					restName="mcadcThrottleRest"
					activeName="mcadcThrottleActive"
					values={values}
					errors={errors}
					handleChange={handleChange}
				/>
				<CalibrationRow
					label="Brake"
					restName="mcadcBrakeRest"
					activeName="mcadcBrakeActive"
					values={values}
					errors={errors}
					handleChange={handleChange}
				/>

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
