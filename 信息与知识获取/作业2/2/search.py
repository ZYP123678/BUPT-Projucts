import math
import re
import os
import numpy as np
from PIL import Image
from io import BytesIO
import base64
from collections import Counter


class ResultItem:
    def __init__(self, index, file_path, text):
        self.index = index
        self.file_path = file_path
        
        # 解析BBC文章元数据
        title_match = re.search(r'标题: (.*?)(\n\n|$)', text)
        url_match = re.search(r'链接: (.*?)(\n\n|$)', text)
        content_match = re.search(r'内容:\s*([\s\S]*)', text)
        
        self.title = title_match.group(1) if title_match else os.path.basename(file_path)
        
        # 增强链接提取 - 多种方式尝试获取链接
        if url_match and url_match.group(1).strip():
            self.url = url_match.group(1).strip()
        else:
            # 尝试直接从文本中查找BBC格式链接
            bbc_url_match = re.search(r'https?://www\.bbc\.co\.uk/news/[a-z0-9/-]+', text)
            if bbc_url_match:
                self.url = bbc_url_match.group(0)
            else:
                # 再尝试查找任何URL
                any_url_match = re.search(r'https?://\S+', text)
                self.url = any_url_match.group(0) if any_url_match else ""
        
        # 提取文本内容，处理OCR文本
        if content_match:
            main_content = content_match.group(1)
            # 检查是否有OCR文本
            ocr_match = re.search(r'\[OCR_TEXT\] (.*?)($|\n\n)', main_content)
            if ocr_match:
                self.ocr_text = ocr_match.group(1)
                # 移除OCR文本，保留主要内容
                self.text = main_content.replace(ocr_match.group(0), '')
            else:
                self.ocr_text = ""
                self.text = main_content
        else:
            self.text = text
            self.ocr_text = ""
        
        self.is_video = "/videos/" in file_path or "视频" in text
        
        # 相关图片字段
        self.related_photos = []
        self.has_ocr_match = False  # 标记是否在OCR文本中有匹配
        
        # 评分相关字段 - 保留原字段并添加BM25相关字段
        self.rank = 0.0            # 原排名得分(兼容性)
        self.bm25_score = 0.0      # BM25得分
        self.freq = 0.0            # 词频统计
        self.count = 0.0           # 匹配词数
        self.similarity = 0.0      # 向量相似度
        self.total_relevance = 0.0 # 总体相关度
        self.occurrence = []       # 匹配位置
        self.is_phrase_match = False # 短语匹配标记

    def __str__(self):
        """格式化显示搜索结果"""
        s = f"标题: {self.title}\n"
        s += f"类型: {'视频' if self.is_video else '文章'}\n"
        s += f"链接: {self.url if self.url else '[未提供]'}\n"
        s += f"相关度: {self.total_relevance:.2f}/100 (BM25: {self.bm25_score:.2f}, 匹配词数: {self.count})\n"
        
        if self.has_ocr_match:
            s += "*** 图片OCR文本匹配 ***\n"
        
        s += "内容片段:\n"
        
        # 显示匹配上下文
        shown_contexts = set()
        for j in self.occurrence[:3]:  # 只显示前3个匹配位置
            try:
                context = self.text[max(0, j[0] - 50):min(len(self.text), j[1] + 50)]
                context_hash = hash(context)
                if context_hash not in shown_contexts:
                    s += f"> ...{context}...\n"
                    shown_contexts.add(context_hash)
            except (IndexError, TypeError):
                continue
        
        # 如果有OCR匹配，显示OCR文本
        if self.has_ocr_match and self.ocr_text:
            s += "\nOCR文本匹配:\n"
            s += f"> {self.ocr_text[:300]}...\n" if len(self.ocr_text) > 300 else f"> {self.ocr_text}\n"
        
        return s


class PhotoResultItem:
    """图片搜索结果项"""
    def __init__(self, photo_path, ocr_text, score=0):
        self.photo_path = photo_path
        self.file_name = os.path.basename(photo_path)
        self.ocr_text = ocr_text
        self.score = score
        self.related_article = None  # 相关文章信息
        self.match_positions = []    # OCR文本中的匹配位置
        
    def to_html(self):
        """将图片结果转换为HTML显示"""
        # 生成图片的base64编码（小型预览版本）
        try:
            with open(self.photo_path, 'rb') as f:
                img_data = f.read()
                img_type = self.photo_path.split('.')[-1].lower()
                if img_type == 'jpg' or img_type == 'jpeg':
                    mime_type = 'image/jpeg'
                elif img_type == 'png':
                    mime_type = 'image/png'
                elif img_type == 'gif':
                    mime_type = 'image/gif'
                else:
                    mime_type = 'image/jpeg'  # 默认类型
                
                # 创建缩略图
                img = Image.open(BytesIO(img_data))
                img.thumbnail((300, 300))  # 调整大小为缩略图
                buffer = BytesIO()
                img.save(buffer, format=img_type.upper())
                base64_data = base64.b64encode(buffer.getvalue()).decode('utf-8')
                img_src = f"data:{mime_type};base64,{base64_data}"
        except Exception as e:
            print(f"处理图片 {self.photo_path} 失败: {e}")
            img_src = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAASwAAAEsCAYAAAB5fY51AAAACXBIWXMAAA7EAAAOxAGVKw4bAAADsklEQVR4nO3BMQEAAADCoPVPbQwfoAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgJcBEPgAAcbL4cMAAAAASUVORK5CYII="
        
        # 创建HTML
        html = f"""
        <div style="border: 1px solid #ddd; border-radius: 8px; overflow: hidden; margin-bottom: 15px; background: white;">
            <div style="display: flex;">
                <div style="flex: 0 0 300px;">
                    <img src="{img_src}" style="width: 100%; height: 250px; object-fit: contain; padding: 10px;" 
                         onclick="window.open('{self.photo_path}', '_blank');" />
                </div>
                <div style="flex: 1; padding: 15px; overflow: hidden;">
                    <h3 style="margin-top: 0; color: #333;">{self.file_name}</h3>
                    <p style="font-size: 13px; color: #777;">相关度得分: {self.score:.1f}</p>
                    <div style="background: #f5f5f5; padding: 10px; border-radius: 5px; max-height: 150px; overflow-y: auto;">
                        <p style="font-size: 13px; margin: 0;"><b>OCR文本:</b> {self.ocr_text[:500] + '...' if len(self.ocr_text) > 500 else self.ocr_text}</p>
                    </div>
        """
        
        if self.related_article:
            article_title = self.related_article.get('title', '相关文章')
            article_url = self.related_article.get('url', '')
            html += f"""
                    <div style="margin-top: 10px;">
                        <p style="font-size: 13px; margin: 0;"><b>关联文章:</b> {article_title}</p>
                        {f'<a href="{article_url}" target="_blank" style="font-size: 12px; color: #0066cc;">查看文章</a>' if article_url else ''}
                    </div>
            """
        
        html += """
                </div>
            </div>
        </div>
        """
        
        return html


# 保留原有相似度计算函数
def get_similarity(a, b):
    """计算向量余弦相似度"""
    # 防止分母为零
    try:
        a_norm = math.sqrt(a.dot(a.T)) 
        b_norm = math.sqrt(b.dot(b.T))
        
        if a_norm == 0 or b_norm == 0:
            return 0
            
        return float(a.dot(b.T)) / (a_norm * b_norm)
    except Exception as e:
        print(f"计算相似度时出错: {e}")
        return 0


# 新增：自适应BM25参数管理类
class AdaptiveBM25Parameters:
    """自适应调整BM25参数的类"""
    def __init__(self, default_k1=1.5, default_b=0.75):
        self.default_k1 = default_k1
        self.default_b = default_b
        self.user_params = {}  # 用户特定参数
        self.learning_rate = 0.1  # 学习率
        
    def get_params(self, user_id="default"):
        """获取用户特定的BM25参数"""
        if user_id not in self.user_params:
            self.user_params[user_id] = {
                "k1": self.default_k1,
                "b": self.default_b,
                "similarity_weight": 30,
                "bm25_weight": 25,
                "count_weight": 15
            }
        return self.user_params[user_id]
    
    def update_params(self, user_id, feedback, text_list):
        """根据用户反馈更新参数"""
        if not feedback or len(feedback) < 2:
            return None  # 需要足够的反馈数据
            
        params = self.get_params(user_id)
        relevant_docs = [doc for doc, is_relevant in feedback.items() if is_relevant == 1]
        non_relevant_docs = [doc for doc, is_relevant in feedback.items() if is_relevant == 0]
        
        if not relevant_docs or not non_relevant_docs:
            return None  # 需要正反两类反馈
        
        # 计算反馈平衡指数 (0-1，越接近1越平衡)
        total_feedback = len(relevant_docs) + len(non_relevant_docs)
        balance_ratio = min(len(relevant_docs), len(non_relevant_docs)) / (total_feedback / 2)
        
        # 动态调整学习率 - 反馈越不平衡，学习率越小
        effective_learning_rate = self.learning_rate * (0.5 + 0.5 * balance_ratio)
        
        # 分析相关文档特征
        avg_rel_len = get_avg_doc_length(relevant_docs, text_list)
        avg_nonrel_len = get_avg_doc_length(non_relevant_docs, text_list)
        
        # 调整b参数 (文档长度归一化) - 幅度减小
        len_diff = abs(avg_rel_len - avg_nonrel_len) / max(avg_rel_len, avg_nonrel_len)
        if len_diff > 0.3:  # 长度差异明显
            # 长文档更相关，减小b使长文档受益
            if avg_rel_len > avg_nonrel_len:
                params["b"] = max(0.4, params["b"] - effective_learning_rate * 0.7)
            # 短文档更相关，增大b惩罚长文档
            else:
                params["b"] = min(0.9, params["b"] + effective_learning_rate * 0.7)
        
        # 调整k1参数 (词频饱和) - 幅度减小
        if avg_rel_len > avg_nonrel_len * 1.5:  # 相关文档明显更长
            # 增大k1，让高词频文档获益更多
            params["k1"] = min(2.5, params["k1"] + effective_learning_rate * 0.6)
        elif avg_nonrel_len > avg_rel_len:  # 不相关文档反而更长
            # 减小k1，减轻词频的影响
            params["k1"] = max(0.7, params["k1"] - effective_learning_rate * 0.6)
        
        # 调整权重参数 - 减小调整幅度
        rel_count = len(relevant_docs)
        nonrel_count = len(non_relevant_docs)
        
        # 计算权重调整倍数 - 根据反馈平衡度调整
        weight_adjust_factor = 3.0 * balance_ratio  # 最大3.0，最小接近0
        
        if rel_count > nonrel_count * 1.5:  # 用户倾向于标记相关文档
            # 增加相似度权重，减少精确匹配权重 - 幅度减小
            params["similarity_weight"] += effective_learning_rate * weight_adjust_factor
            params["count_weight"] -= effective_learning_rate * weight_adjust_factor * 0.4
        else:  # 用户倾向于标记不相关文档
            # 增加精确匹配权重，减少相似度权重 - 幅度减小
            params["count_weight"] += effective_learning_rate * weight_adjust_factor
            params["similarity_weight"] -= effective_learning_rate * weight_adjust_factor * 0.4
        
        # 参数保护机制 - 防止权重过高或过低
        params["similarity_weight"] = max(15, min(45, params["similarity_weight"]))
        params["bm25_weight"] = max(15, min(40, params["bm25_weight"]))
        params["count_weight"] = max(10, min(35, params["count_weight"]))
        
        # 确保权重总和保持一致
        total_weight = params["similarity_weight"] + params["bm25_weight"] + params["count_weight"]
        factor = 70 / total_weight
        params["similarity_weight"] = params["similarity_weight"] * factor
        params["bm25_weight"] = params["bm25_weight"] * factor
        params["count_weight"] = params["count_weight"] * factor
        
        # 保存更新后的参数
        self.user_params[user_id] = params
        return params


# 辅助函数：获取文档平均长度
def get_avg_doc_length(doc_indices, text_list):
    """计算一组文档的平均长度"""
    if not doc_indices or not text_list:
        return 0
    
    try:
        lengths = [len(tokenize_simple(text_list[idx])) for idx in doc_indices if idx < len(text_list)]
        return sum(lengths) / len(lengths) if lengths else 0
    except Exception as e:
        print(f"计算平均文档长度时出错: {e}")
        return 0


# BM25算法核心实现 - 修改为支持自定义参数
def compute_bm25_scores(query_tokens, doc_tokens_list, k1=1.5, b=0.75):
    """计算BM25分数"""
    # 文档数量
    N = len(doc_tokens_list)
    
    # 计算平均文档长度
    doc_lengths = [len(doc) for doc in doc_tokens_list]
    avg_doc_len = sum(doc_lengths) / N if N > 0 else 0
    
    # 计算每个查询词的IDF值
    query_term_counts = Counter(query_tokens)
    query_terms = list(query_term_counts.keys())
    
    # 计算文档频率(包含词项的文档数)
    df = {}
    for term in query_terms:
        df[term] = sum(1 for doc in doc_tokens_list if term in doc)
    
    # 计算IDF
    idf = {}
    for term in query_terms:
        # 平滑处理，避免除零错误
        idf[term] = math.log((N - df[term] + 0.5) / (df[term] + 0.5) + 1.0)
    
    # 计算每个文档的BM25分数
    scores = []
    for i, doc in enumerate(doc_tokens_list):
        score = 0.0
        doc_term_counts = Counter(doc)
        doc_len = doc_lengths[i]
        
        for term in query_terms:
            if term in doc_term_counts:
                tf = doc_term_counts[term]
                # BM25公式
                term_score = idf.get(term, 0) * ((tf * (k1 + 1)) / 
                                               (tf + k1 * (1 - b + b * doc_len / avg_doc_len)))
                score += term_score
        
        scores.append(score)
    
    return scores


def tokenize_simple(text):
    """简单的分词函数，将文本转换为词列表"""
    # 转为小写并分词
    words = re.findall(r'\b\w+\b', text.lower())
    # 过滤短词和数字
    return [word for word in words if len(word) > 1 and not word.isdigit()]


def run_search(search_str, inverse_index, files, text_list, bag, count, feedback=None, feedback_features=None, feedback_terms=None, bm25_params=None):
    """执行结合BM25的搜索并排名结果 - 支持用户反馈和自适应参数"""
    import re

    # 处理搜索词
    search_str = search_str.lower()
    s_list = search_str.split()

    global query_settings
    if 'query_settings' not in globals():
        query_settings = {}

    # 倒排索引检索候选文档
    temp = []
    for s in s_list:
        temp.append(inverse_index.get(s, {}))

    candidate_docs = set()
    for t in temp:
        candidate_docs.update(t.keys())

    if not candidate_docs and len(s_list) > 0:
        try:
            search_vec = bag.transform([search_str]).toarray()
            similarity_scores = []
            for i in range(len(text_list)):
                sim = get_similarity(search_vec[0], count[i].A[0])
                if sim > 0.1:
                    similarity_scores.append((i, sim))
            similarity_scores.sort(key=lambda x: -x[1])
            candidate_docs = {idx for idx, _ in similarity_scores[:10]}
        except Exception as e:
            print(f"相似度搜索失败: {e}")

    if not candidate_docs:
        return []

    query_tokens = tokenize_simple(search_str)
    candidate_indices = sorted(list(candidate_docs))
    doc_tokens_list = [tokenize_simple(text_list[idx]) for idx in candidate_indices]

    # --- 参数读取与初始化 ---
    if search_str not in query_settings:
        query_settings[search_str] = {
            "params": bm25_params.copy() if bm25_params else {},
            "max_results": 20
        }

    current_params = query_settings[search_str]["params"].copy()

    # 合并新传入参数
    if bm25_params:
        for key, value in bm25_params.items():
            if key not in current_params:
                current_params[key] = value

    k1 = current_params.get("k1", 1.5)
    b = current_params.get("b", 0.75)

    bm25_scores = compute_bm25_scores(query_tokens, doc_tokens_list, k1=k1, b=b)

    result_dict = {}

    for index, t in enumerate(temp):
        for j, (file_index, word_count, positions) in t.items():
            if file_index not in result_dict:
                try:
                    item = ResultItem(file_index, files[file_index], text_list[file_index])
                    bm25_idx = candidate_indices.index(file_index) if file_index in candidate_indices else -1
                    item.bm25_score = bm25_scores[bm25_idx] if bm25_idx >= 0 else 0.0
                    result_dict[file_index] = item
                except Exception as e:
                    print(f"创建ResultItem时出错: {e}")
                    continue
            item = result_dict[file_index]
            try:
                item.count += 1
                item.freq += word_count
                item.occurrence.extend(positions)
                if item.ocr_text and search_str in item.ocr_text.lower():
                    item.has_ocr_match = True
                    item.bm25_score *= 1.5
            except Exception as e:
                print(f"处理文档 {file_index} 评分时出错: {e}")

    for idx in candidate_docs:
        if idx not in result_dict:
            try:
                item = ResultItem(idx, files[idx], text_list[idx])
                bm25_idx = candidate_indices.index(idx) if idx in candidate_indices else -1
                item.bm25_score = bm25_scores[bm25_idx] if bm25_idx >= 0 else 0.0
                item.count = 0.1
                result_dict[idx] = item
                if item.ocr_text and any(s in item.ocr_text.lower() for s in s_list):
                    item.has_ocr_match = True
                    item.bm25_score *= 1.5
            except Exception as e:
                print(f"创建相似度匹配ResultItem时出错: {e}")

    if len(s_list) > 1:
        try:
            for file_index, item in list(result_dict.items()):
                if item.count >= len(s_list):
                    full_text = text_list[file_index].lower()
                    if ' '.join(s_list) in full_text:
                        item.is_phrase_match = True
                        item.bm25_score *= 2.0
                        try:
                            for m in re.finditer(r'\b{}\b'.format(re.escape(' '.join(s_list))), full_text):
                                item.occurrence.insert(0, m.span())
                        except:
                            pass
        except Exception as e:
            print(f"处理短语匹配时出错: {e}")

    result_list = list(result_dict.values())
    if not result_list:
        return []

    try:
        search_vec = bag.transform([search_str]).toarray()
        for i in result_list:
            i.similarity = get_similarity(search_vec[0], count[i.index].A[0])
    except Exception as e:
        print(f"计算相似度时出错: {e}")

    similarity_weight = current_params.get("similarity_weight", 30)
    bm25_weight = current_params.get("bm25_weight", 25)
    count_weight = current_params.get("count_weight", 15)

    for item in result_list:
        ocr_bonus = 15.0 if item.has_ocr_match else 0
        query_term_count = len(s_list)
        match_ratio = min(1.0, item.count / max(1, query_term_count))
        match_penalty = 0.3 + 0.7 * match_ratio
        raw_score = (
            item.bm25_score * bm25_weight +
            item.similarity * similarity_weight +
            item.count * count_weight +
            (30.0 if item.is_phrase_match else 0) +
            ocr_bonus
        )
        item.total_relevance = raw_score * match_penalty
        item.total_relevance = min(100, item.total_relevance * 100 / 30)
        if query_term_count > 1 and item.count <= 1:
            item.total_relevance = min(65, item.total_relevance)

        item.rank = item.bm25_score
        item.feedback_adjusted = False
        item.has_feedback = False
        item.param_info = {
            "k1": k1,
            "b": b,
            "similarity_weight": similarity_weight,
            "bm25_weight": bm25_weight,
            "count_weight": count_weight
        }

    if feedback and (len(feedback) > 0 or (feedback_features and feedback_terms)):
        adjust_scores_with_feedback(result_list, feedback, feedback_features, count, bag, text_list, feedback_terms)

    result_list.sort(key=lambda x: (-x.count / max(1, len(s_list)), -x.total_relevance, -x.bm25_score))

    # 阈值过滤
    base_threshold = 20.0
    if len(s_list) > 1:
        base_threshold += 10.0
    not_relevant_count = sum(1 for v in feedback.values() if v == 0) if feedback else 0
    if feedback:
        threshold_boost = min(50, not_relevant_count * 5) / 100
        base_threshold *= (1 + threshold_boost)
    filtered_results = [item for item in result_list if item.total_relevance >= base_threshold]
    if len(s_list) > 1 and len(filtered_results) > 5:
        multi_word_matches = [item for item in filtered_results if item.count > 1]
        if len(multi_word_matches) >= 3:
            filtered_results = multi_word_matches

    # --- 动态最大返回数调整 ---
    temp_max_results = query_settings[search_str]["max_results"]
    print(f"当前最大返回数: {temp_max_results}，搜索词: {search_str}")

    if feedback:
        relevant_count = sum(1 for v in feedback.values() if v == 1)
        not_relevant_ratio = sum(1 for v in feedback.values() if v == 0) / max(1, len(feedback))
        if relevant_count > 0:
            temp_max_results += int(relevant_count * 3)
        if not_relevant_ratio > 0.2:
            temp_max_results = max(3, temp_max_results - int(not_relevant_count * 2))
    # 更新设置
    query_settings[search_str]["params"] = current_params.copy()
    query_settings[search_str]["max_results"] = temp_max_results

    return filtered_results[:temp_max_results]




def adjust_scores_with_feedback(result_list, feedback, feedback_features, count, bag, text_list, feedback_terms=None):
    """根据用户反馈调整搜索结果的分数"""
    if not feedback or not result_list:
        return
    
    # 1. 调整已有评估的文档分数
    for item in result_list:
        doc_index = item.index
        
        # 对已经评估过的文档进行分数调整
        if doc_index in feedback:
            if feedback[doc_index] == 1:  # 标记为相关
                # 提高相关文档的分数
                item.total_relevance = min(100, item.total_relevance * 1.5)
                item.feedback_adjusted = True
                item.has_feedback = True
            else:  # 标记为不相关
                # 降低不相关文档的分数
                item.total_relevance = max(0, item.total_relevance * 0.5)
                item.feedback_adjusted = True
                item.has_feedback = True
    
    # 2. 使用相关性反馈特征向量调整其他文档的分数
    if feedback_features and 'positive' in feedback_features and feedback_features.get('count_pos', 0) > 0:
        pos_features = feedback_features['positive'] / feedback_features['count_pos']
        
        # 计算平均不相关文档特征向量
        neg_features = None
        if 'negative' in feedback_features and feedback_features.get('count_neg', 0) > 0:
            neg_features = feedback_features['negative'] / feedback_features['count_neg']
        
        # 对每个搜索结果计算与相关文档的相似度
        for item in result_list:
            doc_index = item.index
            if doc_index not in feedback:  # 只处理未评估的文档
                try:
                    # 获取文档特征向量
                    doc_vector = count[doc_index].A[0]
                    
                    # 计算与相关文档的相似度
                    pos_similarity = cosine_similarity(doc_vector, pos_features)
                    
                    # 如果有不相关文档，计算与不相关文档的相似度并减去
                    neg_similarity = 0
                    if neg_features is not None:
                        neg_similarity = cosine_similarity(doc_vector, neg_features)
                    
                    # 计算综合相似度 (Rocchio算法简化版)
                    feedback_similarity = pos_similarity - 0.5 * neg_similarity
                    
                    # 调整文档得分
                    feedback_boost = max(0, feedback_similarity * 25)  # 将相似度转换为加分
                    item.total_relevance = min(100, item.total_relevance + feedback_boost)
                    
                    if feedback_boost > 0:
                        item.feedback_adjusted = True
                except Exception as e:
                    print(f"反馈调整得分时出错: {e}")


def cosine_similarity(vec1, vec2):
    """计算两个向量的余弦相似度"""
    try:
        norm1 = np.linalg.norm(vec1)
        norm2 = np.linalg.norm(vec2)
        
        if norm1 == 0 or norm2 == 0:
            return 0
        
        return np.dot(vec1, vec2) / (norm1 * norm2)
    except Exception as e:
        print(f"计算余弦相似度时出错: {e}")
        return 0


# 保留原有图片搜索函数
def search_photos_by_ocr(search_str, photo_metadata, metadata=None):
    """根据OCR文本搜索图片"""
    if not search_str.strip() or not photo_metadata:
        return []
    
    # 搜索词小写
    search_str = search_str.lower()
    search_words = search_str.split()
    
    # 搜索结果
    results = []
    
    # 遍历所有图片
    for photo in photo_metadata:
        ocr_text = photo.get('ocr_text', '').lower()
        if not ocr_text:
            continue
        
        # 计算匹配分数
        score = 0
        
        # 检查完整短语匹配
        if search_str in ocr_text:
            score += 50  # 完整短语加高分
            
            # 尝试找到匹配位置
            try:
                positions = [(m.start(), m.end()) for m in re.finditer(re.escape(search_str), ocr_text)]
            except:
                positions = []
        else:
            positions = []
            
        # 单词匹配
        for word in search_words:
            if len(word) > 2:
                word_count = ocr_text.count(word)
                if word_count > 0:
                    score += word_count * 5  # 每个匹配词加分
        
        # 如果有匹配，添加到结果
        if score > 0:
            result = PhotoResultItem(
                photo_path=photo.get('photo_path', ''),
                ocr_text=photo.get('ocr_text', ''),
                score=score
            )
            result.match_positions = positions
            
            # 尝试查找关联的文章
            if metadata:
                for article in metadata:
                    related_photos = article.get('related_photos', [])
                    if result.photo_path in related_photos:
                        result.related_article = {
                            'title': article.get('title', ''),
                            'url': article.get('url', ''),
                            'file_path': article.get('file_path', '')
                        }
                        break
            
            results.append(result)
    
    # 按分数排序
    results.sort(key=lambda x: -x.score)
    
    return results