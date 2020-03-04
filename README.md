# Smart contract of trading data on eosio blockchain

## Specification

### Tables

#### data

Data registration and storing information

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | data_id | 데이터 번호 |
| string | datatype_name | 데이터 타입명 |
| name | provider | 데이터 제공자 |
| uint64_t | timestamp | 등록 완료 시점 타임스탬프 (단위: 초) |
| asset | price | 가격 ex) “2.000 OSB” |
| uint64_t | status | 상태 값 (0: 제공 중, 1:등록 중, 2: 삭제됨, 3: 보관 종료) |
| string[] | detail_fields | 메타데이터 필드 값 |
| uint64_t | period | 보관 기간 (단위: 일) |
| string | data_hash_original | 데이터 원본 해시 값 |
| uint64_t | size | 데이터 원본 사이즈 (단위: byte) |
| fragment[] | fragments | 데이터 조각 보관 내역 |
| uint64_t | daySingleIDFSfee | 1일 기준 각 보관소 데이터 보관 비용 |
| uint64_t | totalStorageFee | 보관 기간 기준 데이터 총 보관 비용 |

fragment

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | fragment_no | 데이터 조각 번호 (1부터 시작) |
| uint64_t | size | 데이터 조각 사이즈 (단위: byte) |
| string | hash_original | 데이터 조각 원본 해시 값 |
| string | hash_encrypted | 암호화 된 데이터 조각 해시 값 |
| string | encrypt_key | 암호화 키 |
| uint64_t | idfs_cluster_id | 보관중인 IDFS Cluster id |
| string | cid | 데이터 조각 IPFS 해시 값 (CID) |


#### datatype

Data type registration information

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | datatype_id | 데이터 타입 번호 |
| string | datatype_name | 데이터 타입 명 |
| name | definer | 데이터 타입 정의자 |
| uint64_t | detail_fields_num | 메타데이터 필드 개수 |
| string[] | detail_fields | 메타데이터 필드 명 |


#### buyhistory

Data type registration information

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | buy_id | 구매 이력 번호 |
| name | buyer | 구매자 계정 명 |
| uint64_t | data_id | 구매한 데이터 id |
| uint64_t | timestamp | 구매 시점 타임스탬프 (단위: 초) |
| string | buyer_key | 데이터 사용 권한 증명을 위한 공개키 |


#### idfs

Data keeper registration information

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | idfs_id | 보관자 id |
| name | account | 보관자 계정 명 |
| string | idfs_public_key | 키 암호화용 공개키 |
| uint64_t | capacity | 보관 가능한 총 용량 (단위: byte) |
| time_t | since | 보관자 등록 일시 |
| uint64_t | cluster_id | 보관소 id |
| string | ipaddr | IDFS 노드 ip주소 |
| uint64_t | port | IDFS 노드 포트 번호 |


#### idfscluster

Data keeper cluster registration information

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | cluster_id | 보관소 id |
| string | cluster_key_hash | ipfs cluster secret key의 hash값 |
| uint64_t | usage | 현재 보관중인 용량 (단위: byte) |
| uint64_t | capacity | 보관 가능한 총 용량 (단위: byte) |

#### keeperreward

Balance information of data keeper's reward

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | reward_id | 보상 id |
| uint64_t | data_id | 데이터 id |
| uint64_t | fragment_no | 보관중인 데이터 조각 번호 |
| uint64_t | cluster_id | 보관소 id |
| uint64_t | idfs_account | 보관자 account |
| uint64_t | reward_total | 해당 데이터 보관 총 보상 |
| uint64_t | reward_claimed | 정산 받은 보상 |

#### keeperclaim

Settlement history of data keeper's reward

| Type | Field name | Description |
| ---- | ---------- | ----------- |
| uint64_t | claim_id | 정산 id |
| uint64_t | reward_id | 보상 id |
| asset | quantity | 정산 금액 |
| uint64_t | timestamp | 정산 시점 타임스탬프 (단위: 초) |


### Actions

#### adddatabegin

데이터 등록 신청

```
adddatabegin(name provider,			//데이터 제공자 계정명
    std::string datatypename,			//데이터 타입명
    asset price,				//데이터 구매 가격 (단위: 0.0001 OSB)
    vector<std::string> detail_fields,		//데이터 추가 필드 값
    uint64_t period,				//데이터 보관 기간 (단위: 일)
    std::string data_hash_original,		//원본 데이터 해시 값 (해싱 알고리즘: SHA256)
    uint64_t size,				//원본 데이터 사이즈 (단위: byte)
    vector<fragment> fragments,		    //데이터 조각 (size, hash, idfs_cluster_id 정의)
)
```
* 데이터 등록 비용은 1일 기준 1MB 단위당 0.001 OSB 로 계산됨

#### adddataend

데이터 등록 완료

```
adddataend(name provider,		//데이터 제공자 계정명
        uint64_t data_id,		//데이터 id
        vector<fragment> fragments	//각 data fragment (cid, encrypt_key 정의)
)
```
* adddatabegin 메소드에서 계산된 데이터 등록 비용은 provider 계정에서 차감되어 osb.trader 계정으로 송금됨 


#### adddatatype

데이터 타입 추가

```
adddatatype( name user,			    //데이터 타입 정의자 계정명
        std::string datatype_name,		//데이터 타입명
        uint64_t detail_fields_num,		//해당 데이터 타입이 사용할 추가 필드 수
        std::vector<std::string> detail_fields	//데이터 추가 메타데이터 필드명
)
```

#### buydata

데이터 접근 권한 획득

```
buydata( name user,			//데이터 구매자 계정명
        uint64_t data_id,		//구매하고자 하는 데이터 id
        eosio::public_key buyer_key,	//데이터 사용 권한 증명을 위한 공개키
)
```
* 구매하려는 데이터의 가격만큼 토큰을 보유하고 있지 않을 경우 eosio_assert_failure 발생
* 이미 삭제 된 데이터를 구매 시도 했을 경우 “The data is not on sale”라는 메시지와 함께 eosio_assert_failure발생
* 이미 구매한 데이터를 구매 시도 했을 경우 “The data has already been purchased”라는 메시지와 함께 eosio_assert_failure발생


#### removedata

데이터 삭제

```
removedata(name user,		       //데이터 삭제하는 자의 계정명
        uint64_t data_id		//삭제하고자 하는 데이터 id
)
```

* 데이터 삭제는 데이터 등록자만 가능
* 등록자가 아닌 타인이 데이터 삭제를 시도했을 경우 “Only provider can remove data”라는 메시지와 함께 eosio_assert_failure발생
* 이미 삭제한 데이터를 삭제 시도했을 경우 “The data has already been removed”라는 메시지와 함께 eosio_assert_failure발생


#### addidfs

보관자 등록

```
addidfs(name idfs_account,	//IDFS 노드 제공자의 계정명
        uint64_t capacity,	//보관 가능한 스토리지 용량(단위:byte)
        uint64_t cluster_id,	//조인하고자 하는 보관소 id (0일 경우 컨트랙트가 자동 선택?)
        string idfs_public_key,	//키 암호화용 공개키
        string ipaddr,		//IDFS 노드 ip주소
        uint64_t port		//IDFS 노드 포트 번호
)
```

#### addcluster

보관소 등록

```
addcluster(name idfs_account,	    //IDFS 노드 제공자(보관소 등록자)의 계정명
        std::string cluster_key_hash	//보관소의 IPFS cluster secret key의 hash값
)
```

#### claimkreward

보관자 보상 청구

```
claimkreward(name idfs_account,		//보관자 계정명
        uint64_t data_id,		//데이터 id
        uint64_t storagePeriod		//보상받을 보관 기간
)
```

* 보상받을 데이터 보관 비용은 osb.trader 계정에서 차감되어 idfs_account 계정으로 송금됨
* 1일 보관소 보관료 = (1일 데이터 등록 비용 / 데이터 파편화 수) / 5(=클러스터당 보관자 수)


## License

[MIT](./LICENSE)

## Important

See LICENSE for copyright and license terms.  OASISBloc Foundation makes its contribution on a voluntary basis as a member of the OSBIO community and is not responsible for ensuring the overall performance of the software or any related applications.  We make no representation, warranty, guarantee or undertaking in respect of the software or any related documentation, whether expressed or implied, including but not limited to the warranties or merchantability, fitness for a particular purpose and noninfringement. In no event shall we be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or documentation or the use or other dealings in the software or documentation.  Any test results or performance figures are indicative and will not reflect performance under all conditions.  Any reference to any third party or third-party product, service or other resource is not an endorsement or recommendation by Block.one.  We are not responsible, and disclaim any and all responsibility and liability, for your use of or reliance on any of these resources. Third-party resources may be updated, changed or terminated at any time, so the information here may be out of date or inaccurate.
